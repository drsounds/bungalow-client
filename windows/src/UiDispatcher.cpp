#include "UiDispatcher.h"

#include <Windows.h>

namespace space {
namespace win {

namespace {
    constexpr UINT kRunMessage = WM_APP + 41;
    HWND g_window = nullptr;

    LRESULT CALLBACK dispatcherProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (msg == kRunMessage) {
            std::function<void()>* fn = reinterpret_cast<std::function<void()>*>(lParam);
            if (fn) {
                (*fn)();
                delete fn;
            }
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

void initUiDispatcher() {
    if (g_window) return;
    WNDCLASSW wc{};
    wc.lpfnWndProc = dispatcherProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"BungalowUiDispatcher";
    RegisterClassW(&wc);
    g_window = CreateWindowExW(0, wc.lpszClassName, L"", 0, 0, 0, 0, 0,
                               HWND_MESSAGE, nullptr, wc.hInstance, nullptr);
}

void postToUi(std::function<void()> fn) {
    if (!g_window) return;
    auto* heap = new std::function<void()>(std::move(fn));
    if (!PostMessageW(g_window, kRunMessage, 0, reinterpret_cast<LPARAM>(heap)))
        delete heap;
}

} // namespace win
} // namespace space
