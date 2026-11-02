#include "CefBrowserWindow.h"
#include "CefRuntimeInternal.h"

#include "include/base/cef_callback.h"
#include "include/wrapper/cef_closure_task.h"

#include <algorithm>

namespace space {
namespace cefhost {
namespace {

// Sets prefers-color-scheme (and the native control/DevTools theme) for every
// browser of the context. CEF UI thread.
void applyColorScheme(CefRefPtr<CefRequestContext> context, const std::shared_ptr<WindowState>& window) {
    bool dark;
    {
        std::lock_guard<std::mutex> lock(window->mutex);
        dark = window->darkColorScheme;
    }
    context->SetChromeColorScheme(dark ? CEF_COLOR_VARIANT_DARK : CEF_COLOR_VARIANT_LIGHT, 0);
}

} // namespace

void WindowState::apply() {
    if (!window) return;
    SetWindowPos(window, nullptr, bounds.left, bounds.top,
                 bounds.right - bounds.left, bounds.bottom - bounds.top,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS |
                 (visible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
}

void ColorSchemeContextHandler::OnRequestContextInitialized(CefRefPtr<CefRequestContext> context) {
    applyColorScheme(context, m_window);
}

void showDevToolsWindow(CefRefPtr<CefBrowser> browser, const std::string& title, const CefPoint& inspectAt) {
    CefWindowInfo windowInfo;
    windowInfo.SetAsPopup(nullptr, "DevTools - " + title);
    browser->GetHost()->ShowDevTools(windowInfo, nullptr, CefBrowserSettings(), inspectAt);
}

// ── BrowserLifeSpanHandler ────────────────────────────────────────────────────

void BrowserLifeSpanHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
    bool closed;
    std::string pendingUrl;
    {
        std::lock_guard<std::mutex> lock(m_window->mutex);
        m_window->browser = browser;
        m_window->window = browser->GetHost()->GetWindowHandle();
        closed = m_window->closed;
        if (!closed) m_window->apply();
        pendingUrl.swap(m_window->pendingUrl);
    }
    browserCreated(browser);
    // Outside the lock: on this thread CloseBrowser may run OnBeforeClose
    // synchronously.
    if (closed)
        browser->GetHost()->CloseBrowser(true);
    else if (!pendingUrl.empty())
        browser->GetMainFrame()->LoadURL(pendingUrl);
}

// Returning false would make CEF close our top-level frame. Instead finish
// the close by destroying just the browser window, on this (its owner)
// thread once DoClose has returned.
bool BrowserLifeSpanHandler::DoClose(CefRefPtr<CefBrowser> browser) {
    HWND window = browser->GetHost()->GetWindowHandle();
    CefPostTask(TID_UI, base::BindOnce([](HWND w) { if (IsWindow(w)) DestroyWindow(w); }, window));
    return true;
}

void BrowserLifeSpanHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
    {
        std::lock_guard<std::mutex> lock(m_window->mutex);
        m_window->browser = nullptr;
        m_window->window = nullptr;
    }
    browserClosed(browser);
}

// ── BrowserWindow ─────────────────────────────────────────────────────────────

BrowserWindow::BrowserWindow(HWND parent, bool darkColorScheme)
    : m_parent(parent)
    , m_window(std::make_shared<WindowState>()) {
    m_window->darkColorScheme = darkColorScheme;
}

BrowserWindow::~BrowserWindow() {
    CefRefPtr<CefBrowser> browser;
    {
        std::lock_guard<std::mutex> lock(m_window->mutex);
        m_window->closed = true; // also covers a browser still being created
        browser = m_window->browser;
    }
    if (browser) browser->GetHost()->CloseBrowser(true);
}

bool BrowserWindow::isRunning() const {
    std::lock_guard<std::mutex> lock(m_window->mutex);
    return m_window->browser != nullptr;
}

void BrowserWindow::create(CefRefPtr<CefClient> client, const std::string& url,
                           CefRefPtr<CefRequestContext> context, CefRefPtr<CefDictionaryValue> extraInfo) {
    if (!beginBrowserCreation()) return;
    m_created = true;
    m_context = context;

    RECT bounds;
    {
        std::lock_guard<std::mutex> lock(m_window->mutex);
        bounds = m_window->bounds;
    }
    CefWindowInfo windowInfo;
    windowInfo.SetAsChild(m_parent, CefRect(bounds.left, bounds.top,
                                            bounds.right - bounds.left, bounds.bottom - bounds.top));
    windowInfo.runtime_style = CEF_RUNTIME_STYLE_ALLOY;

    if (!CefBrowserHost::CreateBrowser(windowInfo, client, url, CefBrowserSettings(), extraInfo, context)) {
        abortBrowserCreation();
        m_created = false;
    }
}

bool BrowserWindow::load(const std::string& url) {
    std::lock_guard<std::mutex> lock(m_window->mutex);
    if (m_window->browser) {
        m_window->browser->GetMainFrame()->LoadURL(url);
        return true;
    }
    if (m_created) {
        m_window->pendingUrl = url;
        return true;
    }
    return false;
}

void BrowserWindow::setBounds(int x, int y, int width, int height) {
    std::lock_guard<std::mutex> lock(m_window->mutex);
    RECT bounds{x, y, x + (std::max)(width, 0), y + (std::max)(height, 0)};
    if (EqualRect(&bounds, &m_window->bounds)) return;
    m_window->bounds = bounds;
    m_window->apply();
}

void BrowserWindow::setVisible(bool visible) {
    std::lock_guard<std::mutex> lock(m_window->mutex);
    if (visible == m_window->visible) return;
    m_window->visible = visible;
    m_window->apply();
}

void BrowserWindow::executeScript(const std::string& js) {
    std::lock_guard<std::mutex> lock(m_window->mutex);
    if (!m_window->browser) return;
    CefRefPtr<CefFrame> frame = m_window->browser->GetMainFrame();
    frame->ExecuteJavaScript(js, frame->GetURL(), 0);
}

void BrowserWindow::reload() {
    std::lock_guard<std::mutex> lock(m_window->mutex);
    if (m_window->browser) m_window->browser->Reload();
}

void BrowserWindow::showDevTools(const std::string& title) {
    std::lock_guard<std::mutex> lock(m_window->mutex);
    if (!m_window->browser) return;
    showDevToolsWindow(m_window->browser, title, CefPoint());
}

void BrowserWindow::setDarkColorScheme(bool dark) {
    {
        std::lock_guard<std::mutex> lock(m_window->mutex);
        if (dark == m_window->darkColorScheme) return;
        m_window->darkColorScheme = dark;
    }
    if (m_context)
        CefPostTask(TID_UI, base::BindOnce(&applyColorScheme, m_context, m_window));
}

} // namespace cefhost
} // namespace space
