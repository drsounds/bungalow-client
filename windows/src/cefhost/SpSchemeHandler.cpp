#include "SpSchemeHandler.h"

#include "SPAppManager.h"

#include "include/cef_parser.h"
#include "include/wrapper/cef_stream_resource_handler.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace space {
namespace cefhost {
namespace {

fs::path fromUtf8(const std::string& s) {
    return fs::path(reinterpret_cast<const char8_t*>(s.c_str()));
}

CefRefPtr<CefResourceHandler> errorResponse(int status, const char* statusText) {
    static char kBody[] = "";
    return new CefStreamResourceHandler(status, statusText, "text/plain", {},
                                        CefStreamReader::CreateForData(kBody, 0));
}

// Maps a request path inside the bundle to the file to serve, or an empty path.
fs::path findFile(const std::string& bundleRoot, const std::string& relativePath) {
    const std::string resolved = SPAppManager::resolveInBundle(bundleRoot, relativePath);
    if (resolved.empty()) return {};

    std::error_code ec;
    const fs::path target = fromUtf8(resolved);
    if (fs::is_regular_file(target, ec)) return target;
    if (fs::is_directory(target, ec) && fs::is_regular_file(target / L"index.html", ec))
        return target / L"index.html";

    // Client-side route of a single-page app: serve the app shell.
    if (!target.has_extension()) {
        const fs::path shell = fromUtf8(bundleRoot) / L"index.html";
        if (fs::is_regular_file(shell, ec)) return shell;
    }
    return {};
}

} // namespace

SpSchemeHandlerFactory::SpSchemeHandlerFactory(std::string bundleRoot, bool allowAnyOrigin)
    : m_bundleRoot(std::move(bundleRoot))
    , m_allowAnyOrigin(allowAnyOrigin) {}

CefRefPtr<CefResourceHandler> SpSchemeHandlerFactory::Create(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>,
                                                             const CefString&,
                                                             CefRefPtr<CefRequest> request) {
    CefURLParts parts;
    if (!CefParseURL(request->GetURL(), parts))
        return errorResponse(400, "Bad Request");

    // Keep %2F escaped so it cannot introduce path separators.
    std::string path = CefURIDecode(CefString(&parts.path), true,
        static_cast<cef_uri_unescape_rule_t>(UU_SPACES | UU_URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS)).ToString();
    while (!path.empty() && path.front() == '/')
        path.erase(0, 1);

    const fs::path file = findFile(m_bundleRoot, path);
    if (file.empty())
        return errorResponse(404, "Not Found");

    CefRefPtr<CefStreamReader> stream = CefStreamReader::CreateForFile(file.wstring());
    if (!stream)
        return errorResponse(404, "Not Found");

    std::wstring extension = file.extension().wstring();
    if (!extension.empty()) extension.erase(0, 1);
    std::string mimeType = CefGetMimeType(extension).ToString();
    if (mimeType.empty()) mimeType = "application/octet-stream";

    CefResponse::HeaderMap headers;
    headers.emplace("Cache-Control", "no-cache");
    if (m_allowAnyOrigin)
        headers.emplace("Access-Control-Allow-Origin", "*");
    return new CefStreamResourceHandler(200, "OK", mimeType, headers, stream);
}

} // namespace cefhost
} // namespace space
