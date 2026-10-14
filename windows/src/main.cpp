#include <Windows.h>
#include <windowsx.h>

#include "NativeApplication.h"

namespace
{
    constexpr wchar_t windowClassName[] = L"BungalowNativeWin32Window";

    LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto *app = reinterpret_cast<space::NativeApplication *>(GetWindowLongPtrW(window, GWLP_USERDATA));
        switch (message)
        {
        case WM_NCCREATE:
        {
            const auto *create = reinterpret_cast<CREATESTRUCTW *>(lParam);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
            return DefWindowProcW(window, message, wParam, lParam);
        }
        case WM_CREATE:
            if (app && !app->initialize(window))
                return -1;
            return 0;
        case WM_SIZE:
            if (app)
                app->resize();
            InvalidateRect(window, nullptr, FALSE);
            return 0;
        case WM_PAINT:
        {
            PAINTSTRUCT paint{};
            HDC dc = BeginPaint(window, &paint);
            if (app)
                app->paint(dc);
            EndPaint(window, &paint);
            return 0;
        }
        case WM_LBUTTONDOWN:
            SetCapture(window);
            if (app)
                app->mouseEvent("mousedown", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_LBUTTONUP:
            ReleaseCapture();
            if (app)
                app->mouseEvent("mouseup", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_MOUSEMOVE:
            if (app)
                app->mouseEvent("mousemove", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_TIMER:
            if (app)
                app->timer();
            return 0;
        case WM_COMMAND:
            if (app)
                app->command(wParam, lParam);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(window, message, wParam, lParam);
        }
    }
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand)
{
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.hInstance = instance;
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.lpszClassName = windowClassName;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (!RegisterClassExW(&windowClass))
        return static_cast<int>(GetLastError());

    space::NativeApplication app;
    HWND window = CreateWindowExW(0, windowClassName, L"Bungalow Native",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1000, 700,
        nullptr, nullptr, instance, &app);
    if (!window)
        return static_cast<int>(GetLastError());

    ShowWindow(window, showCommand);
    UpdateWindow(window);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}
