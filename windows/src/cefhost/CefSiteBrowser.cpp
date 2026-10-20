#include "SPEmbeddedBrowser.h"

#include "cefhost/CefRuntime.h"
#include "CefBrowserWindow.h"
#include "CefRuntimeInternal.h"

#include "SPElement.h"
#include "UiDispatcher.h"
#include "Win32GraphicsContext.h"

#include "include/cef_parser.h"

#include <filesystem>
#include <set>

namespace fs = std::filesystem;

namespace space {
namespace cefhost {
namespace {

using Callbacks = SPEmbeddedSiteBrowser::Callbacks;

// Profile shared by every site browser, inside the root cache path.
constexpr wchar_t kSitesProfile[] = L"sites";

bool startsWith(const std::string& s, const std::string& prefix) {
    return s.rfind(prefix, 0) == 0;
}

bool isBungalowUri(const std::string& uri) {
    return startsWith(uri, "bungalow:") || startsWith(uri, "space:") ||
           startsWith(uri, "bungalow:") || startsWith(uri, "musik:");
}

bool isWebUrl(const std::string& url) {
    return startsWith(url, "https://") || startsWith(url, "http://");
}

std::string hostOf(const std::string& url) {
    CefURLParts parts;
    if (!CefParseURL(url, parts)) return {};
    return CefString(&parts.host).ToString();
}

// Hosts whose top-level pages may load without asking the host element.
// Touched by the UI thread and the CEF UI thread.
struct AllowedHosts {
    std::mutex mutex;
    std::set<std::string> hosts;

    void add(const std::string& host) {
        if (host.empty()) return;
        std::lock_guard<std::mutex> lock(mutex);
        hosts.insert(host);
    }

    bool contains(const std::string& host) {
        std::lock_guard<std::mutex> lock(mutex);
        return hosts.count(host) != 0;
    }
};

// CEF-side handlers of one site browser. Top-level navigations stay in the
// browser for allowed hosts; bungalow: links, new windows and pages on other
// hosts are handed to the host element. Subframes and redirects are not
// filtered, like in a regular browser.
class SiteClient : public CefClient,
                   public BrowserLifeSpanHandler,
                   public CefDisplayHandler,
                   public CefRequestHandler,
                   public CefContextMenuHandler,
                   public CefDownloadHandler {
public:
    SiteClient(bool developerMode, std::shared_ptr<AllowedHosts> allowedHosts,
               std::shared_ptr<WindowState> window, std::weak_ptr<Callbacks> callbacks)
        : BrowserLifeSpanHandler(std::move(window))
        , m_developerMode(developerMode)
        , m_allowedHosts(std::move(allowedHosts))
        , m_callbacks(std::move(callbacks)) {}

    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
    CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; }
    CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override { return this; }
    CefRefPtr<CefDownloadHandler> GetDownloadHandler() override { return this; }

    // ── Life span ─────────────────────────────────────────────────────────

    // There are no extra windows: target=_blank links and window.open open in
    // the view instead.
    bool OnBeforePopup(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, int, const CefString& targetUrl,
                       const CefString&, CefLifeSpanHandler::WindowOpenDisposition, bool, const CefPopupFeatures&,
                       CefWindowInfo&, CefRefPtr<CefClient>&, CefBrowserSettings&,
                       CefRefPtr<CefDictionaryValue>&, bool*) override {
        handOver(targetUrl);
        return true;
    }

    // ── Display ───────────────────────────────────────────────────────────

    void OnAddressChange(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame> frame, const CefString& url) override {
        if (!frame->IsMain()) return;
        std::string text = url;
        if (!isWebUrl(text)) return;
        post([text](const Callbacks& cb) { if (cb.addressChanged) cb.addressChanged(text); });
    }

    void OnTitleChange(CefRefPtr<CefBrowser>, const CefString& title) override {
        std::string text = title;
        post([text](const Callbacks& cb) { if (cb.titleChanged) cb.titleChanged(text); });
    }

    // ── Requests ──────────────────────────────────────────────────────────

    bool OnBeforeBrowse(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request,
                        bool, bool isRedirect) override {
        const std::string url = request->GetURL();
        if (isBungalowUri(url)) {
            post([url](const Callbacks& cb) { if (cb.navigateRequested) cb.navigateRequested(url); });
            return true;
        }
        if (!frame->IsMain() || isRedirect) return false;
        if (isWebUrl(url)) {
            if (m_allowedHosts->contains(hostOf(url))) return false;
            handOver(url);
            return true;
        }
        // Internal pages only; no file:, chrome: or external protocol handlers.
        return !(startsWith(url, "about:") || startsWith(url, "data:") ||
                 startsWith(url, "blob:") || startsWith(url, "devtools:"));
    }

    // ── Downloads ─────────────────────────────────────────────────────────

    // Downloads go through the app's download manager instead.
    bool CanDownload(CefRefPtr<CefBrowser>, const CefString& url, const CefString&) override {
        std::string text = url;
        if (isWebUrl(text))
            post([text](const Callbacks& cb) { if (cb.downloadRequested) cb.downloadRequested(text); });
        return false;
    }

    // ── Context menu ──────────────────────────────────────────────────────

    // Keeps the default menu, minus view-source (it would need a new window).
    // Developer mode adds the Develop menu commands, like the caption bar's.
    void OnBeforeContextMenu(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, CefRefPtr<CefContextMenuParams>,
                             CefRefPtr<CefMenuModel> model) override {
        model->Remove(MENU_ID_VIEW_SOURCE);
        if (!m_developerMode) return;
        if (model->GetCount() > 0) model->AddSeparator();
        model->AddItem(kShowInspectorCommand, "Show &Inspector");
    }

    bool OnContextMenuCommand(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame>,
                              CefRefPtr<CefContextMenuParams> params, int commandId, EventFlags) override {
        if (commandId != kShowInspectorCommand) return false;
        // Inspect the element that was right-clicked.
        showDevToolsWindow(browser, browser->GetMainFrame()->GetURL(),
                           CefPoint(params->GetXCoord(), params->GetYCoord()));
        return true;
    }

private:
    static constexpr int kShowInspectorCommand = MENU_ID_USER_FIRST;

    const bool m_developerMode;
    const std::shared_ptr<AllowedHosts> m_allowedHosts;
    const std::weak_ptr<Callbacks> m_callbacks;

    // Runs fn on the UI thread unless the browser has been destroyed by then.
    template <typename Fn>
    void post(Fn fn) {
        space::win::postToUi([callbacks = m_callbacks, fn = std::move(fn)]() {
            if (auto cb = callbacks.lock()) fn(*cb);
        });
    }

    void handOver(const std::string& url) {
        if (isBungalowUri(url))
            post([url](const Callbacks& cb) { if (cb.navigateRequested) cb.navigateRequested(url); });
        else if (isWebUrl(url))
            post([url](const Callbacks& cb) { if (cb.navigationRequested) cb.navigationRequested(url); });
    }

    IMPLEMENT_REFCOUNTING(SiteClient);
};

class CefSiteBrowser final : public SPEmbeddedSiteBrowser {
public:
    CefSiteBrowser(HWND parent, Options options, Callbacks callbacks)
        : m_options(std::move(options))
        , m_allowedHosts(std::make_shared<AllowedHosts>())
        , m_callbacks(std::make_shared<Callbacks>(std::move(callbacks)))
        , m_window(parent, m_options.darkColorScheme) {}

    ~CefSiteBrowser() override {
        m_callbacks.reset(); // before m_window closes the browser
    }

    void setBounds(int x, int y, int width, int height) override { m_window.setBounds(x, y, width, height); }
    void setVisible(bool visible) override { m_window.setVisible(visible); }
    void allowHost(const std::string& host) override { m_allowedHosts->add(host); }

    void loadUrl(const std::string& url) override {
        m_allowedHosts->add(hostOf(url));
        if (!m_window.load(url)) create(url);
    }

    void executeScript(const std::string& js) override { m_window.executeScript(js); }
    void reload() override { m_window.reload(); }
    void showDevTools() override { m_window.showDevTools("Site"); }
    void setDarkColorScheme(bool dark) override { m_window.setDarkColorScheme(dark); }

private:
    const Options m_options;
    const std::shared_ptr<AllowedHosts> m_allowedHosts;
    std::shared_ptr<Callbacks> m_callbacks;
    BrowserWindow m_window;

    void create(const std::string& url) {
        // Contexts with the same cache path share cookies and storage.
        CefRequestContextSettings contextSettings;
        CefString(&contextSettings.cache_path) = (fs::path(cacheRoot()) / kSitesProfile).wstring();
        CefRefPtr<CefRequestContext> context =
            CefRequestContext::CreateContext(contextSettings, new ColorSchemeContextHandler(m_window.state()));

        CefRefPtr<CefDictionaryValue> extraInfo = CefDictionaryValue::Create();
        extraInfo->SetString(kStartupScriptKey, m_options.startupScript);
        extraInfo->SetBool(kSiteBrowserKey, true);

        m_window.create(new SiteClient(m_options.developerMode, m_allowedHosts, m_window.state(), m_callbacks),
                        url, context, extraInfo);
    }
};

} // namespace
} // namespace cefhost

std::unique_ptr<SPEmbeddedSiteBrowser> createEmbeddedSiteBrowser(
    SPElement* host, SPEmbeddedSiteBrowser::Options options, SPEmbeddedSiteBrowser::Callbacks callbacks) {
    if (!cefhost::isInitialized() || !host) return nullptr;
    auto* graphics = dynamic_cast<Win32GraphicsContext*>(host->createGraphics());
    if (!graphics || !graphics->getWindow()) return nullptr;
    return std::make_unique<cefhost::CefSiteBrowser>(graphics->getWindow(), std::move(options), std::move(callbacks));
}

} // namespace space
