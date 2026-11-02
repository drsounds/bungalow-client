#pragma once

#include <Windows.h>

#include <memory>
#include <mutex>
#include <string>

#include "include/cef_browser.h"
#include "include/cef_client.h"
#include "include/cef_request_context.h"
#include "include/cef_request_context_handler.h"

// Plumbing shared by the app and site browsers: a CEF browser window parented
// to a Space element's native window and driven from the UI thread.
namespace space {
namespace cefhost {

// Native window state, touched by the UI thread and the CEF UI thread.
struct WindowState {
    std::mutex mutex;
    CefRefPtr<CefBrowser> browser;
    HWND window = nullptr;
    RECT bounds{};
    bool visible = false;
    bool closed = false;
    bool darkColorScheme = false;
    std::string pendingUrl; // requested before the browser finished creating

    // Caller holds mutex. Async so neither thread blocks on the other.
    void apply();
};

// Applies the window's color scheme once its request context is ready, before
// the first page loads.
class ColorSchemeContextHandler : public CefRequestContextHandler {
public:
    explicit ColorSchemeContextHandler(std::shared_ptr<WindowState> window) : m_window(std::move(window)) {}

    void OnRequestContextInitialized(CefRefPtr<CefRequestContext> context) override;

private:
    const std::shared_ptr<WindowState> m_window;

    IMPLEMENT_REFCOUNTING(ColorSchemeContextHandler);
};

// Opens DevTools in its own window; a non-empty point also inspects the
// element there. DevTools shares the browser's request context, so it follows
// its color scheme.
void showDevToolsWindow(CefRefPtr<CefBrowser> browser, const std::string& title, const CefPoint& inspectAt);

// Life span handling of a browser owned by a BrowserWindow. Clients derive
// from it and provide the reference counting.
class BrowserLifeSpanHandler : public CefLifeSpanHandler {
public:
    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
    bool DoClose(CefRefPtr<CefBrowser> browser) override;
    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

protected:
    explicit BrowserLifeSpanHandler(std::shared_ptr<WindowState> window) : m_window(std::move(window)) {}

    const std::shared_ptr<WindowState> m_window;
};

// UI-thread side of one browser window. Creates the browser lazily and
// force-closes it when destroyed.
class BrowserWindow {
public:
    BrowserWindow(HWND parent, bool darkColorScheme);
    ~BrowserWindow();
    BrowserWindow(const BrowserWindow&) = delete;
    BrowserWindow& operator=(const BrowserWindow&) = delete;

    const std::shared_ptr<WindowState>& state() const { return m_window; }
    bool isRunning() const;

    // Creates the browser with url as its first page. The context must have
    // been created with a ColorSchemeContextHandler for state().
    void create(CefRefPtr<CefClient> client, const std::string& url,
                CefRefPtr<CefRequestContext> context, CefRefPtr<CefDictionaryValue> extraInfo);
    // Loads url into the browser, or queues it while it is being created.
    // Returns false if create() has not been called yet.
    bool load(const std::string& url);

    void setBounds(int x, int y, int width, int height);
    void setVisible(bool visible);
    void executeScript(const std::string& js);
    void reload();
    void showDevTools(const std::string& title);
    void setDarkColorScheme(bool dark);

private:
    const HWND m_parent;
    const std::shared_ptr<WindowState> m_window;
    CefRefPtr<CefRequestContext> m_context; // UI thread only
    bool m_created = false; // UI thread only
};

} // namespace cefhost
} // namespace space
