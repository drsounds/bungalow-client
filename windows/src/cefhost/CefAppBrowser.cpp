#include "SPEmbeddedBrowser.h"

#include "cefhost/CefRuntime.h"
#include "CefRuntimeInternal.h"
#include "SpSchemeHandler.h"

#include "SPAppBundle.h"
#include "SPElement.h"
#include "UiDispatcher.h"
#include "Win32GraphicsContext.h"

#include "include/cef_client.h"
#include "include/cef_parser.h"
#include "include/cef_request_context.h"
#include "include/cef_request_context_handler.h"
#include "include/base/cef_callback.h"
#include "include/wrapper/cef_closure_task.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <mutex>

namespace fs = std::filesystem;

namespace space {
namespace cefhost {
namespace {

using Callbacks = SPEmbeddedBrowser::Callbacks;

bool startsWith(const std::string& s, const std::string& prefix) {
    return s.rfind(prefix, 0) == 0;
}

bool isSpacifyUri(const std::string& uri) {
    return startsWith(uri, "spacify:") || startsWith(uri, "space:") ||
           startsWith(uri, "bungalow:") || startsWith(uri, "musik:");
}

// Native window state, touched by the UI thread and the CEF UI thread.
struct WindowState {
    std::mutex mutex;
    CefRefPtr<CefBrowser> browser;
    HWND window = nullptr;
    RECT bounds{};
    bool visible = false;
    bool closed = false;
    std::string pendingUrl; // requested before the browser finished creating

    // Caller holds mutex. Async so neither thread blocks on the other.
    void apply() {
        if (!window) return;
        SetWindowPos(window, nullptr, bounds.left, bounds.top,
                     bounds.right - bounds.left, bounds.bottom - bounds.top,
                     SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS |
                     (visible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
    }
};

// CEF-side handlers of one app browser. Keeps the app inside its bundle:
// documents may only come from sp://<app>/, other requests only from data:,
// blob: and the allowed https/wss hosts.
class AppClient : public CefClient,
                  public CefLifeSpanHandler,
                  public CefDisplayHandler,
                  public CefRequestHandler,
                  public CefResourceRequestHandler,
                  public CefContextMenuHandler {
public:
    AppClient(std::string origin, std::set<std::string> allowedHosts, bool developerMode,
              std::shared_ptr<WindowState> window, std::weak_ptr<Callbacks> callbacks)
        : m_origin(std::move(origin))
        , m_allowedHosts(std::move(allowedHosts))
        , m_developerMode(developerMode)
        , m_window(std::move(window))
        , m_callbacks(std::move(callbacks)) {}

    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
    CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; }
    CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override { return this; }

    // ── Life span ─────────────────────────────────────────────────────────

    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override {
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
    bool DoClose(CefRefPtr<CefBrowser> browser) override {
        HWND window = browser->GetHost()->GetWindowHandle();
        CefPostTask(TID_UI, base::BindOnce([](HWND w) { if (IsWindow(w)) DestroyWindow(w); }, window));
        return true;
    }

    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override {
        {
            std::lock_guard<std::mutex> lock(m_window->mutex);
            m_window->browser = nullptr;
            m_window->window = nullptr;
        }
        browserClosed(browser);
    }

    bool OnBeforePopup(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, int, const CefString& targetUrl,
                       const CefString&, CefLifeSpanHandler::WindowOpenDisposition, bool, const CefPopupFeatures&,
                       CefWindowInfo&, CefRefPtr<CefClient>&, CefBrowserSettings&,
                       CefRefPtr<CefDictionaryValue>&, bool*) override {
        leaveBundle(targetUrl);
        return true;
    }

    // ── Display ───────────────────────────────────────────────────────────

    void OnAddressChange(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame> frame, const CefString& url) override {
        if (!frame->IsMain()) return;
        std::string route = routeOf(url);
        if (route.empty()) return;
        post([route](const Callbacks& cb) { if (cb.routeChanged) cb.routeChanged(route); });
    }

    void OnTitleChange(CefRefPtr<CefBrowser>, const CefString& title) override {
        std::string text = title;
        post([text](const Callbacks& cb) { if (cb.titleChanged) cb.titleChanged(text); });
    }

    // ── Requests ──────────────────────────────────────────────────────────

    bool OnBeforeBrowse(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request,
                        bool, bool) override {
        const std::string url = request->GetURL();
        if (!frame->IsMain() || startsWith(url, m_origin) || startsWith(url, "devtools:"))
            return false;
        leaveBundle(url);
        return true;
    }

    CefRefPtr<CefResourceRequestHandler> GetResourceRequestHandler(
        CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, CefRefPtr<CefRequest>, bool, bool,
        const CefString&, bool&) override {
        return this;
    }

    ReturnValue OnBeforeResourceLoad(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>,
                                     CefRefPtr<CefRequest> request, CefRefPtr<CefCallback>) override {
        return isAllowedResource(request->GetURL()) ? RV_CONTINUE : RV_CANCEL;
    }

    // ── Context menu ──────────────────────────────────────────────────────

    void OnBeforeContextMenu(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, CefRefPtr<CefContextMenuParams>,
                             CefRefPtr<CefMenuModel> model) override {
        if (!m_developerMode) model->Clear();
    }

    // ── Renderer messages ─────────────────────────────────────────────────

    bool OnProcessMessageReceived(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, CefProcessId,
                                  CefRefPtr<CefProcessMessage> message) override {
        if (message->GetName() != kNavigateMessage) return false;
        std::string uri = message->GetArgumentList()->GetString(0);
        if (isSpacifyUri(uri))
            post([uri](const Callbacks& cb) { if (cb.navigateRequested) cb.navigateRequested(uri); });
        return true;
    }

private:
    const std::string m_origin; // "sp://<app>/"
    const std::set<std::string> m_allowedHosts;
    const bool m_developerMode;
    const std::shared_ptr<WindowState> m_window;
    const std::weak_ptr<Callbacks> m_callbacks;

    // Runs fn on the UI thread unless the browser has been destroyed by then.
    template <typename Fn>
    void post(Fn fn) {
        space::win::postToUi([callbacks = m_callbacks, fn = std::move(fn)]() {
            if (auto cb = callbacks.lock()) fn(*cb);
        });
    }

    void leaveBundle(const std::string& url) {
        if (isSpacifyUri(url))
            post([url](const Callbacks& cb) { if (cb.navigateRequested) cb.navigateRequested(url); });
        else if (startsWith(url, "https://") || startsWith(url, "http://"))
            post([url](const Callbacks& cb) { if (cb.externalUrlRequested) cb.externalUrlRequested(url); });
    }

    // sp://app/a/b?x#y -> "/a/b"; empty for URLs outside the bundle.
    std::string routeOf(const std::string& url) const {
        if (!startsWith(url, m_origin)) return {};
        CefURLParts parts;
        if (!CefParseURL(url, parts)) return {};
        std::string path = CefString(&parts.path).ToString();
        if (path.empty() || path == "/index.html") path = "/";
        return path;
    }

    bool isAllowedResource(const std::string& url) const {
        if (startsWith(url, m_origin) || startsWith(url, "data:") || startsWith(url, "blob:") ||
            startsWith(url, "devtools:"))
            return true;
        if (!startsWith(url, "https://") && !startsWith(url, "wss://"))
            return false;
        CefURLParts parts;
        if (!CefParseURL(url, parts)) return false;
        const std::string host = CefString(&parts.host).ToString();
        // An allowed host also covers its subdomains.
        return std::any_of(m_allowedHosts.begin(), m_allowedHosts.end(), [&](const std::string& allowed) {
            return host == allowed ||
                   (host.size() > allowed.size() &&
                    host.compare(host.size() - allowed.size() - 1, std::string::npos, "." + allowed) == 0);
        });
    }

    IMPLEMENT_REFCOUNTING(AppClient);
};

// One app bundle shown in a CEF browser with its own request context, so
// cookies, storage and cache are isolated per app.
class CefAppBrowser final : public SPEmbeddedBrowser {
public:
    CefAppBrowser(const SPAppBundle& bundle, HWND parent, Options options, Callbacks callbacks)
        : m_bundleRoot(bundle.path)
        , m_appId(hostOf(bundle.identifier))
        , m_origin(std::string(kAppScheme) + "://" + m_appId + "/")
        , m_parent(parent)
        , m_options(std::move(options))
        , m_window(std::make_shared<WindowState>())
        , m_callbacks(std::make_shared<Callbacks>(std::move(callbacks))) {}

    ~CefAppBrowser() override {
        m_callbacks.reset();
        CefRefPtr<CefBrowser> browser;
        {
            std::lock_guard<std::mutex> lock(m_window->mutex);
            m_window->closed = true; // also covers a browser still being created
            browser = m_window->browser;
        }
        if (browser) browser->GetHost()->CloseBrowser(true);
    }

    void setBounds(int x, int y, int width, int height) override {
        std::lock_guard<std::mutex> lock(m_window->mutex);
        RECT bounds{x, y, x + (std::max)(width, 0), y + (std::max)(height, 0)};
        if (EqualRect(&bounds, &m_window->bounds)) return;
        m_window->bounds = bounds;
        m_window->apply();
    }

    void setVisible(bool visible) override {
        std::lock_guard<std::mutex> lock(m_window->mutex);
        if (visible == m_window->visible) return;
        m_window->visible = visible;
        m_window->apply();
    }

    void loadPath(const std::string& path) override {
        const std::string url = m_origin + (startsWith(path, "/") ? path.substr(1) : path);
        {
            std::lock_guard<std::mutex> lock(m_window->mutex);
            if (m_window->browser) {
                m_window->browser->GetMainFrame()->LoadURL(url);
                return;
            }
            if (m_created) {
                m_window->pendingUrl = url;
                return;
            }
        }
        create(url);
    }

    void navigateRoute(const std::string& path) override {
        bool running;
        {
            std::lock_guard<std::mutex> lock(m_window->mutex);
            running = m_window->browser != nullptr;
        }
        if (running)
            executeScript("window.navigate(" + nlohmann::json(path).dump() + ");");
        else
            loadPath(path); // not running yet: the route becomes the initial page
    }

    void executeScript(const std::string& js) override {
        std::lock_guard<std::mutex> lock(m_window->mutex);
        if (!m_window->browser) return;
        CefRefPtr<CefFrame> frame = m_window->browser->GetMainFrame();
        frame->ExecuteJavaScript(js, frame->GetURL(), 0);
    }

    void showDevTools() override {
        std::lock_guard<std::mutex> lock(m_window->mutex);
        if (!m_window->browser) return;
        CefWindowInfo windowInfo;
        windowInfo.SetAsPopup(nullptr, "DevTools - " + m_appId);
        m_window->browser->GetHost()->ShowDevTools(windowInfo, nullptr, CefBrowserSettings(), CefPoint());
    }

private:
    const std::string m_bundleRoot;
    const std::string m_appId;
    const std::string m_origin;
    const HWND m_parent;
    const Options m_options;
    std::shared_ptr<WindowState> m_window;
    std::shared_ptr<Callbacks> m_callbacks;
    bool m_created = false; // UI thread only

    // Standard-scheme hosts are canonicalized to lower case.
    static std::string hostOf(std::string identifier) {
        std::transform(identifier.begin(), identifier.end(), identifier.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return identifier;
    }

    void create(const std::string& url) {
        if (!beginBrowserCreation()) return;
        m_created = true;

        // CEF only creates disk profiles directly inside the root cache path.
        CefRequestContextSettings contextSettings;
        CefString(&contextSettings.cache_path) =
            (fs::path(cacheRoot()) / fs::path(reinterpret_cast<const char8_t*>(("app." + m_appId).c_str()))).wstring();
        CefRefPtr<CefRequestContext> context = CefRequestContext::CreateContext(contextSettings, nullptr);
        context->RegisterSchemeHandlerFactory(kAppScheme, m_appId, new SpSchemeHandlerFactory(m_bundleRoot));

        RECT bounds;
        {
            std::lock_guard<std::mutex> lock(m_window->mutex);
            bounds = m_window->bounds;
        }
        CefWindowInfo windowInfo;
        windowInfo.SetAsChild(m_parent, CefRect(bounds.left, bounds.top,
                                                bounds.right - bounds.left, bounds.bottom - bounds.top));
        windowInfo.runtime_style = CEF_RUNTIME_STYLE_ALLOY;

        CefRefPtr<CefDictionaryValue> extraInfo = CefDictionaryValue::Create();
        extraInfo->SetString(kStartupScriptKey, m_options.startupScript);

        CefRefPtr<AppClient> client = new AppClient(m_origin, m_options.allowedHosts, m_options.developerMode,
                                                    m_window, m_callbacks);
        if (!CefBrowserHost::CreateBrowser(windowInfo, client, url, CefBrowserSettings(), extraInfo, context)) {
            abortBrowserCreation();
            m_created = false;
        }
    }
};

} // namespace
} // namespace cefhost

std::unique_ptr<SPEmbeddedBrowser> createEmbeddedAppBrowser(
    const SPAppBundle& bundle, SPElement* host,
    SPEmbeddedBrowser::Options options, SPEmbeddedBrowser::Callbacks callbacks) {
    if (!cefhost::isInitialized() || !host) return nullptr;
    auto* graphics = dynamic_cast<Win32GraphicsContext*>(host->createGraphics());
    if (!graphics || !graphics->getWindow()) return nullptr;
    return std::make_unique<cefhost::CefAppBrowser>(bundle, graphics->getWindow(),
                                                    std::move(options), std::move(callbacks));
}

} // namespace space
