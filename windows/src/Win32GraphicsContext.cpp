// GDI+ goes first: Space headers pull in `using namespace std`, which makes
// the SDK's `byte` ambiguous with std::byte.
#include <algorithm>
#include <Windows.h>
#include <objidl.h>

// gdiplus.h relies on min/max, which NOMINMAX removes.
namespace Gdiplus
{
    using std::min;
    using std::max;
}
#include <gdiplus.h>

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

        Gdiplus::Color gdiplusColor(SPColor color)
        {
            auto clamp = [](short value) { return static_cast<BYTE>(std::clamp<int>(value, 0, 255)); };
            return Gdiplus::Color(clamp(color.getA()), clamp(color.getR()), clamp(color.getG()), clamp(color.getB()));
        }

        // GDI+ honours the HDC's clip region and viewport origin. Pixel centres sit at .5
        // so integer rectangles land exactly on pixel boundaries, like plain GDI.
        void prepare(Gdiplus::Graphics& graphics, bool antiAlias)
        {
            graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
            graphics.SetSmoothingMode(antiAlias ? Gdiplus::SmoothingModeAntiAlias : Gdiplus::SmoothingModeNone);
        }

        // Clamps the radius so short elements become pills instead of the
        // overlapping-arc artifacts GDI RoundRect produces.
        void roundedRectanglePath(Gdiplus::GraphicsPath& path, Gdiplus::REAL x, Gdiplus::REAL y,
                                  Gdiplus::REAL width, Gdiplus::REAL height, Gdiplus::REAL radius)
        {
            radius = std::min(radius, std::min(width, height) / 2.0f);
            if (radius <= 0.5f)
            {
                path.AddRectangle(Gdiplus::RectF(x, y, width, height));
                return;
            }
            const Gdiplus::REAL d = radius * 2.0f;
            path.AddArc(x, y, d, d, 180.0f, 90.0f);
            path.AddArc(x + width - d, y, d, d, 270.0f, 90.0f);
            path.AddArc(x + width - d, y + height - d, d, d, 0.0f, 90.0f);
            path.AddArc(x, y + height - d, d, d, 90.0f, 90.0f);
            path.CloseFigure();
        }

        void fillGradient(Gdiplus::Graphics& graphics, const Gdiplus::GraphicsPath& path,
                          int x, int y, int width, int height, const SPGradient& gradient)
        {
            if (width <= 0 || height <= 0)
                return;
            const bool horizontal = gradient.getDirection() != 0;
            // Pad the brush by a pixel; GDI+ otherwise wraps the end colour onto the first row.
            const Gdiplus::RectF brushRect(
                static_cast<Gdiplus::REAL>(horizontal ? x - 1 : x),
                static_cast<Gdiplus::REAL>(horizontal ? y : y - 1),
                static_cast<Gdiplus::REAL>(horizontal ? width + 2 : width),
                static_cast<Gdiplus::REAL>(horizontal ? height : height + 2));
            Gdiplus::LinearGradientBrush brush(brushRect, gdiplusColor(gradient.getStart()), gdiplusColor(gradient.getEnd()),
                horizontal ? Gdiplus::LinearGradientModeHorizontal : Gdiplus::LinearGradientModeVertical);
            brush.SetWrapMode(Gdiplus::WrapModeTileFlipXY);
            graphics.FillPath(&brush, &path);
        }

        class GdiplusSession
        {
        public:
            GdiplusSession()
            {
                Gdiplus::GdiplusStartupInput input;
                Gdiplus::GdiplusStartup(&token, &input, nullptr);
            }
            ~GdiplusSession()
            {
                if (token)
                    Gdiplus::GdiplusShutdown(token);
            }
        private:
            ULONG_PTR token = 0;
        };

        // Bounds the cache when sizes or colours churn (window resizes, live colour sliders).
        constexpr size_t kMaxCachedBitmaps = 512;
        constexpr size_t kMaxCachedBytes = 64 * 1024 * 1024;

        std::uint32_t packColor(SPColor color)
        {
            auto clamp = [](short value) { return static_cast<std::uint32_t>(std::clamp<int>(value, 0, 255)); };
            return clamp(color.getA()) << 24 | clamp(color.getR()) << 16 | clamp(color.getG()) << 8 | clamp(color.getB());
        }

        // Top-down 32bpp DIB; pixels are premultiplied BGRA as AlphaBlend expects.
        HBITMAP createDib(int width, int height, void **bits)
        {
            BITMAPINFO info{};
            info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            info.bmiHeader.biWidth = width;
            info.bmiHeader.biHeight = -height;
            info.bmiHeader.biPlanes = 1;
            info.bmiHeader.biBitCount = 32;
            info.bmiHeader.biCompression = BI_RGB;
            HBITMAP bitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, bits, nullptr, 0);
            if (bitmap && *bits)
                memset(*bits, 0, static_cast<size_t>(width) * height * 4);
            return bitmap;
        }
    }

    Win32GraphicsContext::Win32GraphicsContext(HWND window) : window(window)
    {
        static GdiplusSession session;
        cacheDc = CreateCompatibleDC(nullptr);
    }

    Win32GraphicsContext::~Win32GraphicsContext()
    {
        clearCache();
        if (cacheDc)
            DeleteDC(cacheDc);
    }

    void Win32GraphicsContext::themeChanged()
    {
        clearCache();
    }

    void Win32GraphicsContext::clearCache()
    {
        for (auto& entry : cache)
            DeleteObject(entry.second.bitmap);
        cache.clear();
        cacheBytes = 0;
    }


    void Win32GraphicsContext::storeBudget(size_t bytes)
    {
        if (cache.size() >= kMaxCachedBitmaps || cacheBytes + bytes > kMaxCachedBytes)
            clearCache();
        cacheBytes += bytes;
    }

    const Win32GraphicsContext::CachedBitmap *Win32GraphicsContext::cachedStrip(
        SPColor start, SPColor end, bool horizontal, int length)
    {
        if (length <= 0)
            return nullptr;
        const CacheKey key{static_cast<int>(CachedKind::Strip), packColor(start), packColor(end),
                           horizontal ? 1 : 0, length, 0, 0};
        if (const auto found = cache.find(key); found != cache.end())
            return &found->second;

        void *bits = nullptr;
        const int width = horizontal ? length : 1;
        const int height = horizontal ? 1 : length;
        HBITMAP bitmap = createDib(width, height, &bits);
        if (!bitmap || !bits)
        {
            if (bitmap)
                DeleteObject(bitmap);
            return nullptr;
        }
        // Interpolate once into a 1px strip; drawing stretches it across the other axis.
        auto *pixels = static_cast<std::uint8_t *>(bits);
        const std::uint32_t a = packColor(start);
        const std::uint32_t b = packColor(end);
        auto channel = [](std::uint32_t color, int shift) { return static_cast<int>((color >> shift) & 0xff); };
        for (int i = 0; i < length; ++i)
        {
            const float t = length > 1 ? static_cast<float>(i) / (length - 1) : 0.0f;
            auto mix = [&](int shift) {
                return channel(a, shift) + (channel(b, shift) - channel(a, shift)) * t;
            };
            const float alpha = mix(24);
            pixels[i * 4] = static_cast<std::uint8_t>(mix(0) * alpha / 255.0f + 0.5f);
            pixels[i * 4 + 1] = static_cast<std::uint8_t>(mix(8) * alpha / 255.0f + 0.5f);
            pixels[i * 4 + 2] = static_cast<std::uint8_t>(mix(16) * alpha / 255.0f + 0.5f);
            pixels[i * 4 + 3] = static_cast<std::uint8_t>(alpha + 0.5f);
        }
        storeBudget(static_cast<size_t>(length) * 4);
        const bool opaque = (a >> 24) == 0xff && (b >> 24) == 0xff;
        return &cache.emplace(key, CachedBitmap{bitmap, width, height, opaque}).first->second;
    }

    const Win32GraphicsContext::CachedBitmap *Win32GraphicsContext::cachedRoundedShape(
        CachedKind kind, const SPGradient& gradient, int width, int height, int radius)
    {
        if (width <= 0 || height <= 0)
            return nullptr;
        const CacheKey key{static_cast<int>(kind), packColor(gradient.getStart()), packColor(gradient.getEnd()),
                           gradient.getDirection() != 0 ? 1 : 0, width, height, radius};
        if (const auto found = cache.find(key); found != cache.end())
            return &found->second;

        void *bits = nullptr;
        HBITMAP bitmap = createDib(width, height, &bits);
        if (!bitmap || !bits)
        {
            if (bitmap)
                DeleteObject(bitmap);
            return nullptr;
        }
        {
            // GDI+ rasterises the anti-aliased shape straight into the DIB's memory, once.
            Gdiplus::Bitmap target(width, height, width * 4, PixelFormat32bppPARGB, static_cast<BYTE *>(bits));
            Gdiplus::Graphics graphics(&target);
            prepare(graphics, true);
            Gdiplus::GraphicsPath path;
            if (kind == CachedKind::RoundedStroke)
            {
                roundedRectanglePath(path, 0.5f, 0.5f, width - 1.0f, height - 1.0f, static_cast<Gdiplus::REAL>(radius));
                Gdiplus::Pen pen(gdiplusColor(gradient.getStart()), 1.0f);
                graphics.DrawPath(&pen, &path);
            }
            else
            {
                roundedRectanglePath(path, 0.0f, 0.0f, static_cast<Gdiplus::REAL>(width),
                                     static_cast<Gdiplus::REAL>(height), static_cast<Gdiplus::REAL>(radius));
                fillGradient(graphics, path, 0, 0, width, height, gradient);
            }
        }
        storeBudget(static_cast<size_t>(width) * height * 4);
        return &cache.emplace(key, CachedBitmap{bitmap, width, height, false}).first->second;
    }

    void Win32GraphicsContext::blit(const CachedBitmap& cached, int x, int y, int width, int height)
    {
        HGDIOBJ old = SelectObject(cacheDc, cached.bitmap);
        if (cached.opaque && cached.width == width && cached.height == height)
        {
            BitBlt(dc, x, y, width, height, cacheDc, 0, 0, SRCCOPY);
        }
        else if (cached.opaque)
        {
            // Strips only ever stretch across the 1px axis, so pixel duplication is exact.
            const int oldMode = SetStretchBltMode(dc, COLORONCOLOR);
            StretchBlt(dc, x, y, width, height, cacheDc, 0, 0, cached.width, cached.height, SRCCOPY);
            SetStretchBltMode(dc, oldMode);
        }
        else
        {
            const BLENDFUNCTION blend{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
            AlphaBlend(dc, x, y, width, height, cacheDc, 0, 0, cached.width, cached.height, blend);
        }
        SelectObject(cacheDc, old);
    }

    void Win32GraphicsContext::fillSolid(int x, int y, int width, int height, SPColor color)
    {
        if (!dc || width <= 0 || height <= 0 || color.getA() <= 0)
            return;
        if (color.getA() >= 255)
        {
            const RECT rect{x, y, x + width, y + height};
            SetDCBrushColor(dc, nativeColor(color));
            FillRect(dc, &rect, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
            return;
        }
        if (const CachedBitmap *pixel = cachedStrip(color, color, false, 1))
            blit(*pixel, x, y, width, height);
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
            // Premultiply for AlphaBlend.
            const unsigned a = image->pixels[i].a;
            pixels[i * 4] = static_cast<unsigned char>(image->pixels[i].b * a / 255);
            pixels[i * 4 + 1] = static_cast<unsigned char>(image->pixels[i].g * a / 255);
            pixels[i * 4 + 2] = static_cast<unsigned char>(image->pixels[i].r * a / 255);
            pixels[i * 4 + 3] = static_cast<unsigned char>(a);
        }
        void *bits = nullptr;
        HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (!bitmap || !bits)
        {
            if (bitmap)
                DeleteObject(bitmap);
            return;
        }
        memcpy(bits, pixels.data(), pixels.size());
        HDC source = CreateCompatibleDC(dc);
        HGDIOBJ old = SelectObject(source, bitmap);
        BLENDFUNCTION blend{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
        AlphaBlend(dc, x, y, width, height, source, x1, y1, x2, y2, blend);
        SelectObject(source, old);
        DeleteDC(source);
        DeleteObject(bitmap);
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
        // Axis-aligned lines (nearly all of them) are 1px solid fills.
        if (x1 == x2 || y1 == y2)
        {
            fillSolid(std::min(x1, x2), std::min(y1, y2), std::abs(x2 - x1) + 1, std::abs(y2 - y1) + 1, color);
            return;
        }
        Gdiplus::Graphics graphics(dc);
        prepare(graphics, true);
        Gdiplus::Pen pen(gdiplusColor(color), 1.0f);
        graphics.DrawLine(&pen, x1 + 0.5f, y1 + 0.5f, x2 + 0.5f, y2 + 0.5f);
    }

    void Win32GraphicsContext::drawRectangle(int x, int y, int width, int height, SPColor color)
    {
        if (!dc || width <= 0 || height <= 0)
            return;
        fillSolid(x, y, width, 1, color);
        if (height > 1)
            fillSolid(x, y + height - 1, width, 1, color);
        if (height > 2)
        {
            fillSolid(x, y + 1, 1, height - 2, color);
            if (width > 1)
                fillSolid(x + width - 1, y + 1, 1, height - 2, color);
        }
    }

    void Win32GraphicsContext::fillRectangle(int x, int y, int width, int height, SPColor color)
    {
        fillSolid(x, y, width, height, color);
    }

    void Win32GraphicsContext::fillRoundedRectangle(int x, int y, int width, int height, int radius, SPColor color)
    {
        if (!dc || width <= 0 || height <= 0)
            return;
        if (radius <= 0)
        {
            fillSolid(x, y, width, height, color);
            return;
        }
        if (const CachedBitmap *shape = cachedRoundedShape(CachedKind::RoundedFill, SPGradient(color, color, 0),
                                                           width, height, radius))
            blit(*shape, x, y, width, height);
    }

    void Win32GraphicsContext::drawRoundedRectangle(int x, int y, int width, int height, int radius, SPColor color)
    {
        if (!dc || width <= 1 || height <= 1)
            return;
        if (radius <= 0)
        {
            drawRectangle(x, y, width, height, color);
            return;
        }
        if (const CachedBitmap *shape = cachedRoundedShape(CachedKind::RoundedStroke, SPGradient(color, color, 0),
                                                           width, height, radius))
            blit(*shape, x, y, width, height);
    }

    void Win32GraphicsContext::fillRoundedLinearGradientRectangle(int x, int y, int width, int height,
        int radius, int direction, const SPGradient& gradient)
    {
        if (!dc || width <= 0 || height <= 0)
            return;
        if (radius <= 0)
        {
            drawLinearGradient({x, y, width, height}, gradient);
            return;
        }
        if (const CachedBitmap *shape = cachedRoundedShape(CachedKind::RoundedFill, gradient, width, height, radius))
            blit(*shape, x, y, width, height);
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
        DrawTextW(dc, value.c_str(), static_cast<int>(value.size()), &rect, DT_LEFT | DT_TOP | DT_NOPREFIX | DT_NOCLIP);
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
        if (rect.width <= 0 || rect.height <= 0)
            return;
        const bool horizontal = gradient.getDirection() != 0;
        if (const CachedBitmap *strip = cachedStrip(gradient.getStart(), gradient.getEnd(), horizontal,
                                                    horizontal ? rect.width : rect.height))
            blit(*strip, rect.x, rect.y, rect.width, rect.height);
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

    void Win32GraphicsContext::scheduleThaw(int milliseconds)
    {
        if (window)
            SetTimer(window, 1, static_cast<UINT>(milliseconds), nullptr);
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
        HDC measureDc = dc ? dc : (window ? GetDC(window) : nullptr);
        if (!measureDc)
            return {};
        const std::wstring value = toWide(text);
        const std::wstring family = toWide(font.family);
        HFONT handle = CreateFontW(-std::max(1, font.height), 0, 0, 0, font.weight,
            font.italic, font.underline, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, family.c_str());
        HGDIOBJ oldFont = SelectObject(measureDc, handle);
        RECT rect{0, 0, 0, 0};
        DrawTextW(measureDc, value.c_str(), static_cast<int>(value.size()), &rect, DT_CALCRECT | DT_NOPREFIX);
        SelectObject(measureDc, oldFont);
        DeleteObject(handle);
        if (!dc)
            ReleaseDC(window, measureDc);
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
