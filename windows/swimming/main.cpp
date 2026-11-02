// swimming: runs a swimming app (a vite-plugin-swimming bundle, an HTML page
// or an http(s) URL) in a Win32 window drawn with Win32GraphicsContext, or,
// with --out, renders one frame to a PNG.
#include <Windows.h>
#include <windowsx.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "SPClassicSkin.h"
#include "SPMouseEventArgs.h"
#include "SPSwimmingElement.h"
#include "SPWindowElement.h"
#include "Space.h"
#include "SwimmingScheduler.h"
#include "Win32GraphicsContext.h"
#include "swimming/Cli.h"

namespace
{
    constexpr wchar_t windowClassName[] = L"SwimmingWin32Window";

    std::string toUtf8(const wchar_t *text)
    {
        const int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
        std::string out(size > 0 ? size - 1 : 0, '\0');
        if (size > 1)
            WideCharToMultiByte(CP_UTF8, 0, text, -1, out.data(), size, nullptr, nullptr);
        return out;
    }

    // One window showing one swimming app.
    struct Viewer
    {
        swimming::CliOptions options;
        HWND window = nullptr;
        space::Space *space = nullptr;
        space::Win32GraphicsContext *graphics = nullptr;
        space::SPWindowElement *windowElement = nullptr;
        space::SPSwimmingElement *app = nullptr;
        bool leftButtonDown = false;

        bool initialize(HWND handle)
        {
            window = handle;
            space = new space::Space();
            graphics = new space::Win32GraphicsContext(window);
            space->setGraphicsContext(graphics);
            windowElement = new space::SPWindowElement(space, new space::SPClassicSkin());
            // Space requires a colour scheme on the window.
            windowElement->setColorScheme(SP_COLOR_SCHEME_LIGHT);
            space->addWindow(windowElement);
            app = new space::SPSwimmingElement(windowElement);
            windowElement->appendChild(app);
            resize();
            try
            {
                app->start(options.source, space::SPSwimmingElement::fetchWith(graphics), swimming::createEngineFor(options));
            }
            catch (const std::exception &error)
            {
                std::cerr << "cannot load app: " << error.what() << std::endl;
                return false;
            }
            return true;
        }

        void resize()
        {
            RECT client{};
            GetClientRect(window, &client);
            windowElement->setAbsoluteBounds({0, 0, client.right, client.bottom});
            app->setX(0);
            app->setY(0);
            app->setWidth((std::max)(0, static_cast<int>(client.right)));
            app->setHeight((std::max)(0, static_cast<int>(client.bottom)));
            app->pack();
        }

        void paint(HDC dc)
        {
            RECT client{};
            GetClientRect(window, &client);
            graphics->setDeviceContext(dc);
            graphics->beginPaint();
            windowElement->draw(graphics, client.right, client.bottom);
            graphics->endPaint();
            graphics->setDeviceContext(nullptr);
        }

        void mouseEvent(const char *name, int x, int y)
        {
            space->dispatchEvent(name, new space::SPMouseEventArgs(0, x, y), windowElement);
        }
    };

    LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto *viewer = reinterpret_cast<Viewer *>(GetWindowLongPtrW(window, GWLP_USERDATA));
        switch (message)
        {
        case WM_NCCREATE:
        {
            const auto *create = reinterpret_cast<CREATESTRUCTW *>(lParam);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
            return DefWindowProcW(window, message, wParam, lParam);
        }
        case WM_CREATE:
            return viewer && viewer->initialize(window) ? 0 : -1;
        case WM_SIZE:
            if (viewer && viewer->app)
                viewer->resize();
            InvalidateRect(window, nullptr, FALSE);
            return 0;
        case WM_PAINT:
        {
            PAINTSTRUCT paint{};
            HDC dc = BeginPaint(window, &paint);
            if (viewer && viewer->app)
                viewer->paint(dc);
            EndPaint(window, &paint);
            return 0;
        }
        case WM_LBUTTONDOWN:
            SetCapture(window);
            if (viewer)
                viewer->mouseEvent("mousedown", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_LBUTTONUP:
            ReleaseCapture();
            if (viewer)
                viewer->mouseEvent("mouseup", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_TIMER:
            if (viewer && viewer->graphics)
                viewer->graphics->thaw();
            return 0;
        case WM_COMMAND:
            if (viewer && viewer->graphics)
                viewer->graphics->handleCommand(wParam, lParam);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(window, message, wParam, lParam);
        }
    }
}

int wmain(int argc, wchar_t **argv)
{
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i)
        args.push_back(toUtf8(argv[i]));
    const auto options = swimming::parseCli(args);
    if (!options)
    {
        std::cerr << swimming::kCliUsage << std::endl;
        return 2;
    }

    if (options->out)
    {
        try
        {
            space::Win32GraphicsContext requests(nullptr);
            swimming::renderFrame(*options, space::SPSwimmingElement::fetchWith(&requests));
        }
        catch (const std::exception &error)
        {
            std::cerr << "cannot render app: " << error.what() << std::endl;
            return 1;
        }
        return 0;
    }

    HINSTANCE instance = GetModuleHandleW(nullptr);
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.hInstance = instance;
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.lpszClassName = windowClassName;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (!RegisterClassExW(&windowClass))
        return static_cast<int>(GetLastError());

    space::win::installSwimmingScheduler();
    Viewer viewer;
    viewer.options = *options;
    RECT frame{0, 0, static_cast<LONG>(options->width), static_cast<LONG>(options->height)};
    AdjustWindowRect(&frame, WS_OVERLAPPEDWINDOW, FALSE);
    HWND window = CreateWindowExW(0, windowClassName, L"Swimming", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                  frame.right - frame.left, frame.bottom - frame.top, nullptr, nullptr, instance, &viewer);
    if (!window)
        return 1;

    ShowWindow(window, SW_SHOWDEFAULT);
    UpdateWindow(window);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}
