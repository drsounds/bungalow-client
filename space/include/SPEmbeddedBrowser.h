#ifndef SP_EMBEDDED_BROWSER_H
#define SP_EMBEDDED_BROWSER_H

#include <functional>
#include <memory>
#include <set>
#include <string>

namespace space {
    struct SPAppBundle;
    class SPElement;

    /// A native browser that hosts one HTML5 app bundle (a bundle with an
    /// index.html) and serves it from sp://<bundle>/. Each instance has its
    /// own isolated storage. Backend-neutral; backends provide
    /// createEmbeddedAppBrowser().
    ///
    /// All methods must be called on the UI thread, and all callbacks are
    /// delivered on the UI thread. Callbacks stop once the browser is
    /// destroyed.
    class SPEmbeddedBrowser {
    public:
        struct Options {
            bool developerMode = false;
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
        virtual void showDevTools() = 0;
    };

    /// Implemented by the active UI backend. host is the element whose native
    /// window the browser is parented to. Returns nullptr if no embedded
    /// browser is available.
    std::unique_ptr<SPEmbeddedBrowser> createEmbeddedAppBrowser(
        const SPAppBundle& bundle, SPElement* host,
        SPEmbeddedBrowser::Options options, SPEmbeddedBrowser::Callbacks callbacks);
}

#endif
