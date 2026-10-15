#include "WinHttpClient.h"

#include <Windows.h>
#include <winhttp.h>

#include <vector>

namespace space {
namespace win {

namespace {
    std::wstring widen(const std::string& s) {
        if (s.empty()) return {};
        int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
        std::wstring w(n > 0 ? n - 1 : 0, L'\0');
        if (n > 0) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), n);
        return w;
    }

    std::string narrow(const std::wstring& w) {
        if (w.empty()) return {};
        int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string s(n > 0 ? n - 1 : 0, '\0');
        if (n > 0) WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, s.data(), n, nullptr, nullptr);
        return s;
    }

    std::string queryHeader(HINTERNET req, DWORD level) {
        DWORD size = 0;
        WinHttpQueryHeaders(req, level, WINHTTP_HEADER_NAME_BY_INDEX, WINHTTP_NO_OUTPUT_BUFFER, &size,
                            WINHTTP_NO_HEADER_INDEX);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || size == 0) return {};
        std::wstring buf(size / sizeof(wchar_t), L'\0');
        if (!WinHttpQueryHeaders(req, level, WINHTTP_HEADER_NAME_BY_INDEX, buf.data(), &size,
                                 WINHTTP_NO_HEADER_INDEX))
            return {};
        buf.resize(size / sizeof(wchar_t));
        return narrow(buf);
    }

    struct Handle {
        HINTERNET h = nullptr;
        ~Handle() { if (h) WinHttpCloseHandle(h); }
    };
}

HttpResult httpRequest(const std::string& url,
                       const std::string& method,
                       const std::function<bool(const char*, size_t)>& onData,
                       const std::atomic<bool>* cancel,
                       const std::function<void(const HttpResult&)>& onHeaders)
{
    HttpResult result;

    std::wstring wurl = widen(url);
    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);
    uc.dwSchemeLength = uc.dwHostNameLength = uc.dwUrlPathLength = uc.dwExtraInfoLength = static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc)) {
        result.error = "Invalid URL";
        return result;
    }

    std::wstring host(uc.lpszHostName, uc.dwHostNameLength);
    std::wstring path(uc.lpszUrlPath, uc.dwUrlPathLength);
    path.append(uc.lpszExtraInfo, uc.dwExtraInfoLength);
    if (path.empty()) path = L"/";

    Handle session, connection, request;
    session.h = WinHttpOpen(L"Spacify/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session.h) { result.error = "WinHttpOpen failed"; return result; }

    connection.h = WinHttpConnect(session.h, host.c_str(), uc.nPort, 0);
    if (!connection.h) { result.error = "WinHttpConnect failed"; return result; }

    DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    request.h = WinHttpOpenRequest(connection.h, widen(method).c_str(), path.c_str(), nullptr,
                                   WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request.h) { result.error = "WinHttpOpenRequest failed"; return result; }

    if (!WinHttpSendRequest(request.h, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(request.h, nullptr)) {
        result.error = "Request failed (" + std::to_string(GetLastError()) + ")";
        return result;
    }

    DWORD status = 0, size = sizeof(status);
    WinHttpQueryHeaders(request.h, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);
    result.status = static_cast<int>(status);
    result.contentType = queryHeader(request.h, WINHTTP_QUERY_CONTENT_TYPE);
    result.contentDisposition = queryHeader(request.h, WINHTTP_QUERY_CONTENT_DISPOSITION);
    std::string len = queryHeader(request.h, WINHTTP_QUERY_CONTENT_LENGTH);
    if (!len.empty()) {
        try { result.contentLength = std::stoll(len); } catch (...) {}
    }
    result.ok = true;
    if (onHeaders) onHeaders(result);

    if (method == "HEAD") return result;

    std::vector<char> buffer(64 * 1024);
    for (;;) {
        if (cancel && cancel->load()) { result.cancelled = true; return result; }
        DWORD read = 0;
        if (!WinHttpReadData(request.h, buffer.data(), static_cast<DWORD>(buffer.size()), &read)) {
            result.ok = false;
            result.error = "Read failed (" + std::to_string(GetLastError()) + ")";
            return result;
        }
        if (read == 0) break;
        if (onData && !onData(buffer.data(), read)) { result.cancelled = true; return result; }
    }
    return result;
}

} // namespace win
} // namespace space
