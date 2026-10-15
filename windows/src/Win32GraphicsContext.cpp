#include "Win32GraphicsContext.h"

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#include <Winhttp.h>
#include <commctrl.h>
#include <wingdi.h>

#include "SPElement.h"
#include "SPImage.h"
#include "SPInputElement.h"
#include "SPInputEventArgs.h"
#include "SPSkin.h"

namespace space
{
    namespace
    {
        std::wstring toWide(const string& value)
        {
            if (value.empty())
                return {};
            const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
            std::wstring result(size, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size);
            return result;
        }

        string toUtf8(const std::wstring& value)
        {
            if (value.empty())
                return {};
            const int size = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                                                  nullptr, 0, nullptr, nullptr);
            string result(size, '\0');
            WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(),
                                size, nullptr, nullptr);
            return result;
        }

        LRESULT CALLBACK inputEditProcedure(HWND edit, UINT message, WPARAM wParam, LPARAM lParam,
                                             UINT_PTR, DWORD_PTR refData)
        {
            if (message == WM_KEYDOWN && wParam == VK_RETURN)
            {
                auto *element = reinterpret_cast<SPInputElement *>(refData);
                const int length = GetWindowTextLengthW(edit);
                std::wstring value(length + 1, L'\0');
                if (length > 0)
                    GetWindowTextW(edit, value.data(), length + 1);
                value.resize(length);
                element->notify("enter", element, new SPInputEventArgs(toUtf8(value)));
                return 0;
            }
            return DefSubclassProc(edit, message, wParam, lParam);
        }

        COLORREF nativeColor(SPColor color)
        {
            return RGB(color.getR(), color.getG(), color.getB());
        }

        TRIVERTEX vertex(int x, int y, SPColor color)
        {
            return {x, y,
                static_cast<COLOR16>(color.getR() * 257),
                static_cast<COLOR16>(color.getG() * 257),
                static_cast<COLOR16>(color.getB() * 257),
                0xffff};
        }
    }

    Win32GraphicsContext::Win32GraphicsContext(HWND window) : window(window)
    {
    }

    void Win32GraphicsContext::setDeviceContext(HDC dc)
    {
        this->dc = dc;
    }

    bool Win32GraphicsContext::handleCommand(WPARAM wParam, LPARAM lParam)
    {
        if (HIWORD(wParam) != EN_CHANGE || lParam == 0)
            return false;
        HWND control = reinterpret_cast<HWND>(lParam);
        const auto found = inputElements.find(control);
        if (found == inputElements.end())
            return false;
        const int length = GetWindowTextLengthW(control);
        std::wstring value(length + 1, L'\0');
        if (length > 0)
            GetWindowTextW(control, value.data(), length + 1);
        value.resize(length);
        if (auto *input = dynamic_cast<SPInputElement *>(found->second))
            input->setValue(toUtf8(value));
        return true;
    }

    void Win32GraphicsContext::setControlAttribute(SPElement *element, const string& name, const string& value)
    {
        if (name != "value" && name != "text")
            return;
        for (const auto& entry : inputElements)
        {
            if (entry.second == element)
            {
                const int length = GetWindowTextLengthW(entry.first);
                std::wstring current(length + 1, L'\0');
                if (length > 0)
                    GetWindowTextW(entry.first, current.data(), length + 1);
                current.resize(length);
                const std::wstring replacement = toWide(value);
                if (current != replacement)
                    SetWindowTextW(entry.first, replacement.c_str());
                return;
            }
        }
    }

    void Win32GraphicsContext::request(const string& method, const string& url, const string& data, RequestCallback callback)
    {
        const std::wstring wideUrl = toWide(url);
        URL_COMPONENTS parts{};
        parts.dwStructSize = sizeof(parts);
        parts.dwSchemeLength = static_cast<DWORD>(-1);
        parts.dwHostNameLength = static_cast<DWORD>(-1);
        parts.dwUrlPathLength = static_cast<DWORD>(-1);
        parts.dwExtraInfoLength = static_cast<DWORD>(-1);
        if (wideUrl.empty() || !WinHttpCrackUrl(wideUrl.c_str(), static_cast<DWORD>(wideUrl.size()), 0, &parts))
        {
            callback(false, string());
            return;
        }

        const std::wstring host(parts.lpszHostName, parts.dwHostNameLength);
        const std::wstring path = parts.dwUrlPathLength
            ? std::wstring(parts.lpszUrlPath, parts.dwUrlPathLength) : L"/";
        const std::wstring extra = parts.dwExtraInfoLength
            ? std::wstring(parts.lpszExtraInfo, parts.dwExtraInfoLength) : std::wstring();
        const std::wstring wideMethod = toWide(method);
        HINTERNET session = WinHttpOpen(L"Bungalow Native/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        HINTERNET connection = session ? WinHttpConnect(session, host.c_str(), parts.nPort, 0) : nullptr;
        HINTERNET request = connection ? WinHttpOpenRequest(connection, wideMethod.c_str(), (path + extra).c_str(),
            nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
            parts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0) : nullptr;

        string response;
        bool success = false;
        const DWORD dataSize = static_cast<DWORD>(data.size());
        if (request && WinHttpSendRequest(request, L"Content-Type: application/json\r\n",
                static_cast<DWORD>(-1), data.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char *>(data.data()),
                dataSize, dataSize, 0) && WinHttpReceiveResponse(request, nullptr))
        {
            DWORD status = 0;
            DWORD statusSize = sizeof(status);
            if (WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                    WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX) &&
                status >= 200 && status < 300)
            {
                DWORD available = 0;
                DWORD read = 0;
                char buffer[4096];
                success = true;
                while (WinHttpQueryDataAvailable(request, &available) && available > 0)
                {
                    const DWORD count = std::min<DWORD>(available, sizeof(buffer));
                    if (!WinHttpReadData(request, buffer, count, &read) || read == 0)
                    {
                        success = false;
                        break;
                    }
                    response.append(buffer, read);
                }
            }
        }

        if (request)
            WinHttpCloseHandle(request);
        if (connection)
            WinHttpCloseHandle(connection);
        if (session)
            WinHttpCloseHandle(session);
        callback(success, response);
    }

    void Win32GraphicsContext::drawImagePart(SPImage *image, int x, int y, int width, int height,
                                              int x1, int y1, int x2, int y2)
    {
        if (!image)
            return;
        const size_t pixelWidth = image->width;
        const size_t pixelHeight = image->height;
        if (pixelWidth == 0 || pixelHeight == 0 ||
            pixelWidth > static_cast<size_t>(std::numeric_limits<LONG>::max()) ||
            pixelHeight > static_cast<size_t>(std::numeric_limits<LONG>::max()) ||
            pixelWidth > std::numeric_limits<size_t>::max() / pixelHeight)
            return;
        const size_t pixelCount = pixelWidth * pixelHeight;
        if (pixelCount > std::numeric_limits<size_t>::max() / 4)
            return;
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = static_cast<LONG>(pixelWidth);
        info.bmiHeader.biHeight = -static_cast<LONG>(pixelHeight);
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        std::vector<unsigned char> pixels(pixelCount * 4);
        for (size_t i = 0; i < pixelCount; ++i)
        {
            pixels[i * 4] = image->pixels[i].b;
            pixels[i * 4 + 1] = image->pixels[i].g;
            pixels[i * 4 + 2] = image->pixels[i].r;
            pixels[i * 4 + 3] = image->pixels[i].a;
        }
        StretchDIBits(dc, x, y, width, height, x1, y1, x2, y2, pixels.data(), &info, DIB_RGB_COLORS, SRCCOPY);
    }

    void Win32GraphicsContext::drawSkinImage(const string& resourceId, int x, int y, int width, int height)
    {
        if (skin)
            drawImage(skin->getImage(resourceId, ""), x, y, width, height);
    }

    void Win32GraphicsContext::setClip(const sp_rectangle& rect)
    {
        if (dc)
        {
            const int state = SaveDC(dc);
            if (state)
                savedClipStates.push_back(state);
            IntersectClipRect(dc, rect.x, rect.y, rect.x + rect.width, rect.y + rect.height);
        }
    }

    void Win32GraphicsContext::declipRect()
    {
        if (dc && !savedClipStates.empty())
        {
            RestoreDC(dc, savedClipStates.back());
            savedClipStates.pop_back();
        }
    }

    void Win32GraphicsContext::drawLine(int x1, int y1, int x2, int y2, SPColor color)
    {
        if (!dc)
            return;
        HPEN pen = CreatePen(PS_SOLID, 1, nativeColor(color));
        HGDIOBJ old = SelectObject(dc, pen);
        MoveToEx(dc, x1, y1, nullptr);
        LineTo(dc, x2, y2);
        SelectObject(dc, old);
        DeleteObject(pen);
    }

    void Win32GraphicsContext::drawRectangle(int x, int y, int width, int height, SPColor color)
    {
        if (!dc)
            return;
        HPEN pen = CreatePen(PS_SOLID, 1, nativeColor(color));
        HGDIOBJ oldPen = SelectObject(dc, pen);
        HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
        Rectangle(dc, x, y, x + width, y + height);
        SelectObject(dc, oldBrush);
        SelectObject(dc, oldPen);
        DeleteObject(pen);
    }

    void Win32GraphicsContext::fillRectangle(int x, int y, int width, int height, SPColor color)
    {
        if (!dc)
            return;
        RECT rect{x, y, x + width, y + height};
        HBRUSH brush = CreateSolidBrush(nativeColor(color));
        FillRect(dc, &rect, brush);
        DeleteObject(brush);
    }

    void Win32GraphicsContext::fillRoundedRectangle(int x, int y, int width, int height, int radius, SPColor color)
    {
        if (!dc)
            return;
        HBRUSH brush = CreateSolidBrush(nativeColor(color));
        HGDIOBJ oldBrush = SelectObject(dc, brush);
        HGDIOBJ oldPen = SelectObject(dc, GetStockObject(NULL_PEN));
        RoundRect(dc, x, y, x + width, y + height, radius * 2, radius * 2);
        SelectObject(dc, oldPen);
        SelectObject(dc, oldBrush);
        DeleteObject(brush);
    }

    void Win32GraphicsContext::drawRoundedRectangle(int x, int y, int width, int height, int radius, SPColor color)
    {
        if (!dc)
            return;
        HPEN pen = CreatePen(PS_SOLID, 1, nativeColor(color));
        HGDIOBJ oldPen = SelectObject(dc, pen);
        HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
        RoundRect(dc, x, y, x + width, y + height, radius * 2, radius * 2);
        SelectObject(dc, oldBrush);
        SelectObject(dc, oldPen);
        DeleteObject(pen);
    }

    void Win32GraphicsContext::fillRoundedLinearGradientRectangle(int x, int y, int width, int height,
        int radius, int direction, const SPGradient& gradient)
    {
        fillRoundedRectangle(x, y, width, height, radius, gradient.getStart());
    }

    void Win32GraphicsContext::drawImage(SPImage *image, int x, int y, int width, int height)
    {
        if (image)
            drawImagePart(image, x, y, width, height, 0, 0, image->width, image->height);
    }

    void Win32GraphicsContext::drawControl(const string& name, SPElement *element, int x, int y, int width, int height)
    {
        if (name == "input")
            drawInputElement(element, x, y, width, height);
        else if (element)
            drawString(element->getInnerText(), element->getFontStyle(), element->getForegroundColor(), x, y, width, height);
    }

    void Win32GraphicsContext::drawString(const string& text, SPFontStyle font, SPColor color,
                                           int x, int y, int width, int height)
    {
        if (!dc)
            return;
        const std::wstring family = toWide(font.family);
        const std::wstring value = toWide(text);
        HFONT handle = CreateFontW(-std::max(1, font.height), 0, 0, 0, font.weight,
            font.italic, font.underline, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, family.c_str());
        HGDIOBJ oldFont = SelectObject(dc, handle);
        const int oldMode = SetBkMode(dc, TRANSPARENT);
        const COLORREF oldColor = SetTextColor(dc, nativeColor(color));
        RECT rect{x, y, x + width, y + height};
        DrawTextW(dc, value.c_str(), static_cast<int>(value.size()), &rect, DT_LEFT | DT_TOP | DT_WORDBREAK);
        SetTextColor(dc, oldColor);
        SetBkMode(dc, oldMode);
        SelectObject(dc, oldFont);
        DeleteObject(handle);
    }

    void Win32GraphicsContext::invalidateRegion(sp_rectangle region)
    {
        RECT rect{region.x, region.y, region.x + region.width, region.y + region.height};
        if (!frozen && window)
            InvalidateRect(window, &rect, FALSE);
    }

    void Win32GraphicsContext::drawHTMLText(const string& html, SPFontStyle font, SPColor color,
                                             int x, int y, int width, int height)
    {
        drawString(html, font, color, x, y, width, height);
    }

    void Win32GraphicsContext::drawLinearGradient(const sp_rectangle& rect, const SPGradient& gradient)
    {
        if (!dc)
            return;
        TRIVERTEX vertices[2] = {
            vertex(rect.x, rect.y, gradient.getStart()),
            vertex(rect.x + rect.width, rect.y + rect.height, gradient.getEnd())
        };
        GRADIENT_RECT gradientRect{0, 1};
        GradientFill(dc, vertices, 2, &gradientRect, 1, gradient.getDirection() == 0 ? GRADIENT_FILL_RECT_V : GRADIENT_FILL_RECT_H);
    }

    SPImage *Win32GraphicsContext::loadImage(const string& bitmap)
    {
        try
        {
            return new SPImage(bitmap);
        }
        catch (...)
        {
            return nullptr;
        }
    }

    void Win32GraphicsContext::freeze()
    {
        frozen = true;
    }

    void Win32GraphicsContext::thaw()
    {
        frozen = false;
        if (window)
        {
            KillTimer(window, 1);
            InvalidateRect(window, nullptr, FALSE);
        }
    }

    sp_rectangle Win32GraphicsContext::measureString(const string& text, const SPFontStyle& font)
    {
        if (!dc)
            return {};
        const std::wstring value = toWide(text);
        const std::wstring family = toWide(font.family);
        HFONT handle = CreateFontW(-std::max(1, font.height), 0, 0, 0, font.weight,
            font.italic, font.underline, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, family.c_str());
        HGDIOBJ oldFont = SelectObject(dc, handle);
        RECT rect{0, 0, 0, 0};
        DrawTextW(dc, value.c_str(), static_cast<int>(value.size()), &rect, DT_CALCRECT | DT_WORDBREAK);
        SelectObject(dc, oldFont);
        DeleteObject(handle);
        return {0, 0, rect.right - rect.left, rect.bottom - rect.top};
    }

    int Win32GraphicsContext::getWidth()
    {
        RECT rect{};
        return window && GetClientRect(window, &rect) ? rect.right - rect.left : 0;
    }

    int Win32GraphicsContext::getHeight()
    {
        RECT rect{};
        return window && GetClientRect(window, &rect) ? rect.bottom - rect.top : 0;
    }

    void Win32GraphicsContext::drawInputElement(SPElement *element, int x, int y, int width, int height)
    {
        if (!element)
            return;
        HWND control = nullptr;
        for (const auto& entry : inputElements)
        {
            if (entry.second == element)
            {
                control = entry.first;
                break;
            }
        }
        if (!control && window)
        {
            control = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP,
                x, y, width, height, window, nullptr, GetModuleHandleW(nullptr), nullptr);
            if (control)
            {
                inputElements.emplace(control, element);
                SetWindowSubclass(control, inputEditProcedure, 1, reinterpret_cast<DWORD_PTR>(element));
            }
        }
        if (!control)
            return;
        SetWindowPos(control, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
        if (GetFocus() != control)
            SetWindowTextW(control, toWide(element->getStringAttribute("value", "")).c_str());
    }

    void Win32GraphicsContext::setOrigo(const int& x, const int& y)
    {
        origin = {x, y};
        if (dc)
            SetViewportOrgEx(dc, x, y, nullptr);
    }

    void Win32GraphicsContext::restoreOrigo()
    {
        origin = {0, 0};
        if (dc)
            SetViewportOrgEx(dc, 0, 0, nullptr);
    }
}
