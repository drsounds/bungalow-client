#ifndef SP_EMBEDDED_BROWSER_H
#define SP_EMBEDDED_BROWSER_H

#include <functional>
#include <memory>
#include <set>
#include <string>

namespace space {
    struct SPAppBundle;
    class SPElement;

    /// Identifier of the bundle with the shared Apps JavaScript API
    /// (Apps/import). Every app browser also serves it at sp://import/, and
    /// its files may be read cross-origin (sp.require() uses XHR).
    inline constexpr char kSharedApiBundle[] = "import";

    /// A native browser that hosts one HTML5 app bundle (a bundle with an
    /// index.html) and serves it from sp://<bundle>/, plus the shared API
    /// bundle (kSharedApiBundle). Each instance has its own isolated storage.
    /// Backend-neutral; backends provide createEmbeddedAppBrowser().
    ///
    /// All methods must be called on the UI thread, and all callbacks are
    /// delivered on the UI thread. Callbacks stop once the browser is
    /// destroyed.
    class SPEmbeddedBrowser {
    public:
        struct Options {
            bool developerMode = false;
            /// Initial color scheme; see setDarkColorScheme().
            bool darkColorScheme = false;
            /// JavaScript run in every app page before its own scripts.
            std::string startupScript;
            /// https/wss hosts the app may fetch from, besides its own bundle.
            std::set<std::string> allowedHosts;
        };

        struct Callbacks {
            /// The app changed its route (pushState, hash change, page load).
            /// path is the in-app path, e.g. "/a/b".
            std::function<void(const std::string& path)> routeChanged;
            /// The app asked the host to open a spacify: URI
            /// (window.spacify.navigate or a spacify: link).
            std::function<void(const std::string& uri)> navigateRequested;
            /// The app tried to open a page outside its bundle.
            std::function<void(const std::string& url)> externalUrlRequested;
            std::function<void(const std::string& title)> titleChanged;
        };

        virtual ~SPEmbeddedBrowser() = default;

        /// Window-relative bounds in pixels.
        virtual void setBounds(int x, int y, int width, int height) = 0;
        virtual void setVisible(bool visible) = 0;
        /// Loads an in-app path ("/a/b" -> sp://<bundle>/a/b).
        virtual void loadPath(const std::string& path) = 0;
        /// Routes the running app to an in-app path via window.navigate(path).
        virtual void navigateRoute(const std::string& path) = 0;
        virtual void executeScript(const std::string& js) = 0;
        virtual void reload() = 0;
        virtual void showDevTools() = 0;
        /// Follows the host's light/dark mode: prefers-color-scheme, native
        /// form controls and scrollbars, and the DevTools theme.
        virtual void setDarkColorScheme(bool dark) = 0;
    };

    /// Implemented by the active UI backend. host is the element whose native
    /// window the browser is parented to. Returns nullptr if no embedded
    /// browser is available.
    std::unique_ptr<SPEmbeddedBrowser> createEmbeddedAppBrowser(
        const SPAppBundle& bundle, SPElement* host,
        SPEmbeddedBrowser::Options options, SPEmbeddedBrowser::Callbacks callbacks);

    /// A native browser that shows ordinary web sites (SiteView). All sites
    /// share one persistent profile. Top-level navigations stay in the browser
    /// only for hosts the host element has allowed; anything else is handed
    /// back through navigationRequested so the host can ask for consent.
    ///
    /// Same threading rules as SPEmbeddedBrowser.
    class SPEmbeddedSiteBrowser {
    public:
        struct Options {
            bool developerMode = false;
            /// Initial color scheme; see setDarkColorScheme().
            bool darkColorScheme = false;
            /// JavaScript run in every top-level page before its own scripts.
            std::string startupScript;
        };

        struct Callbacks {
            /// The main frame's URL changed (page load, pushState, hash change).
            std::function<void(const std::string& url)> addressChanged;
            /// The page asked to open a spacify: URI (link or redirect).
            std::function<void(const std::string& uri)> navigateRequested;
            /// The page tried to open an http(s) page on a host that has not
            /// been allowed, or in a new window. The navigation was cancelled.
            std::function<void(const std::string& url)> navigationRequested;
            /// The page started a download. It was cancelled in the browser.
            std::function<void(const std::string& url)> downloadRequested;
            std::function<void(const std::string& title)> titleChanged;
        };

        virtual ~SPEmbeddedSiteBrowser() = default;

        /// Window-relative bounds in pixels.
        virtual void setBounds(int x, int y, int width, int height) = 0;
        virtual void setVisible(bool visible) = 0;
        /// Lets top-level navigations to host (exact match) stay in the browser.
        virtual void allowHost(const std::string& host) = 0;
        /// Loads url, allowing its host first.
        virtual void loadUrl(const std::string& url) = 0;
        virtual void executeScript(const std::string& js) = 0;
        virtual void reload() = 0;
        virtual void showDevTools() = 0;
        virtual void setDarkColorScheme(bool dark) = 0;
    };

    /// Implemented by the active UI backend; see createEmbeddedAppBrowser().
    std::unique_ptr<SPEmbeddedSiteBrowser> createEmbeddedSiteBrowser(
        SPElement* host, SPEmbeddedSiteBrowser::Options options, SPEmbeddedSiteBrowser::Callbacks callbacks);
}

#endif
