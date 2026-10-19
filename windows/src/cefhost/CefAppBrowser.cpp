#include "SPEmbeddedBrowser.h"

#include "cefhost/CefRuntime.h"
#include "CefBrowserWindow.h"
#include "CefRuntimeInternal.h"
#include "SpSchemeHandler.h"

#include "SPAppBundle.h"
#include "SPAppManager.h"
#include "SPElement.h"
#include "UiDispatcher.h"
#include "Win32GraphicsContext.h"

#include "include/cef_parser.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>

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

// "sp://import/": the shared API bundle every app may load from.
std::string sharedApiOrigin() {
    return std::string(kAppScheme) + "://" + kSharedApiBundle + "/";
}

// CEF-side handlers of one app browser. Keeps the app inside its bundle:
// documents may only come from sp://<app>/, other requests only from the
// shared API bundle, data:, blob: and the allowed https/wss hosts.
class AppClient : public CefClient,
                  public BrowserLifeSpanHandler,
                  public CefDisplayHandler,
                  public CefRequestHandler,
                  public CefResourceRequestHandler,
                  public CefContextMenuHandler {
public:
    AppClient(std::string appId, std::string origin, std::set<std::string> allowedHosts,
              std::shared_ptr<WindowState> window, std::weak_ptr<Callbacks> callbacks)
        : BrowserLifeSpanHandler(std::move(window))
        , m_appId(std::move(appId))
        , m_origin(std::move(origin))
        , m_allowedHosts(std::move(allowedHosts))
        , m_callbacks(std::move(callbacks)) {}

    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
    CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; }
    CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override { return this; }

    // ── Life span ─────────────────────────────────────────────────────────

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

    // Right-clicking an app shows the Develop menu, like the caption bar's.
    void OnBeforeContextMenu(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, CefRefPtr<CefContextMenuParams>,
                             CefRefPtr<CefMenuModel> model) override {
        model->Clear();
        model->AddItem(kShowInspectorCommand, "Show &Inspector");
        model->AddItem(kReloadPageCommand, "&Reload Page");
    }

    bool OnContextMenuCommand(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame>,
                              CefRefPtr<CefContextMenuParams> params, int commandId, EventFlags) override {
        switch (commandId) {
        case kShowInspectorCommand:
            // Inspect the element that was right-clicked.
            showDevToolsWindow(browser, m_appId, CefPoint(params->GetXCoord(), params->GetYCoord()));
            return true;
        case kReloadPageCommand:
            browser->Reload();
            return true;
        default:
            return false;
        }
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
    static constexpr int kShowInspectorCommand = MENU_ID_USER_FIRST;
    static constexpr int kReloadPageCommand = MENU_ID_USER_FIRST + 1;

    const std::string m_appId;
    const std::string m_origin; // "sp://<app>/"
    const std::set<std::string> m_allowedHosts;
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
        if (startsWith(url, m_origin) || startsWith(url, sharedApiOrigin()) || startsWith(url, "data:") ||
            startsWith(url, "blob:") || startsWith(url, "devtools:"))
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
        , m_options(std::move(options))
        , m_callbacks(std::make_shared<Callbacks>(std::move(callbacks)))
        , m_window(parent, m_options.darkColorScheme) {}

    ~CefAppBrowser() override {
        m_callbacks.reset(); // before m_window closes the browser
    }

    void setBounds(int x, int y, int width, int height) override { m_window.setBounds(x, y, width, height); }
    void setVisible(bool visible) override { m_window.setVisible(visible); }

    void loadPath(const std::string& path) override {
        const std::string url = m_origin + (startsWith(path, "/") ? path.substr(1) : path);
        if (!m_window.load(url)) create(url);
    }

    void navigateRoute(const std::string& path) override {
        if (m_window.isRunning())
            executeScript("window.navigate(" + nlohmann::json(path).dump() + ");");
        else
            loadPath(path); // not running yet: the route becomes the initial page
    }

    void executeScript(const std::string& js) override { m_window.executeScript(js); }
    void reload() override { m_window.reload(); }
    void showDevTools() override { m_window.showDevTools(m_appId); }
    void setDarkColorScheme(bool dark) override { m_window.setDarkColorScheme(dark); }

private:
    const std::string m_bundleRoot;
    const std::string m_appId;
    const std::string m_origin;
    const Options m_options;
    std::shared_ptr<Callbacks> m_callbacks;
    BrowserWindow m_window;

    // Standard-scheme hosts are canonicalized to lower case.
    static std::string hostOf(std::string identifier) {
        std::transform(identifier.begin(), identifier.end(), identifier.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return identifier;
    }

    void create(const std::string& url) {
        // CEF only creates disk profiles directly inside the root cache path.
        CefRequestContextSettings contextSettings;
        CefString(&contextSettings.cache_path) =
            (fs::path(cacheRoot()) / fs::path(reinterpret_cast<const char8_t*>(("app." + m_appId).c_str()))).wstring();
        CefRefPtr<CefRequestContext> context =
            CefRequestContext::CreateContext(contextSettings, new ColorSchemeContextHandler(m_window.state()));
        context->RegisterSchemeHandlerFactory(kAppScheme, m_appId, new SpSchemeHandlerFactory(m_bundleRoot));
        if (m_appId != kSharedApiBundle) {
            if (const SPAppBundle* shared = SPAppManager::instance().findBundle(kSharedApiBundle))
                context->RegisterSchemeHandlerFactory(kAppScheme, kSharedApiBundle,
                                                      new SpSchemeHandlerFactory(shared->path, true));
        }

        CefRefPtr<CefDictionaryValue> extraInfo = CefDictionaryValue::Create();
        extraInfo->SetString(kStartupScriptKey, m_options.startupScript);

        m_window.create(new AppClient(m_appId, m_origin, m_options.allowedHosts, m_window.state(), m_callbacks),
                        url, context, extraInfo);
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
