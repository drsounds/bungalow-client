#pragma once
#include <SiteView.h>

#include <map>

#ifdef BUNGALOW_CEF
#include <memory>
#include "SPEmbeddedBrowser.h"
#endif

namespace space {
struct SPAppBundle;
class SPNativeAppElement;
class SPTextElement;

namespace views {

// Like SiteView but handles bungalow:app: URIs.
// Dev mode: full URL bar always shown.
// Non-dev mode: nav bar with "Add app" button shown only while the app is not in favourites.
//
// Installed bundles with "AppType": "Native" run as Lua MVC apps rendered
// with native elements (see SPNativeApp), one per app, kept alive while
// switching so controllers keep their state.
//
// With BUNGALOW_CEF, other installed bundles that contain an index.html run
// locally in an embedded browser (one per app, kept alive while switching),
// served from sp://<app_id>/. bungalow:app:<app_id>:a:b maps to the in-app
// route /a/b. Unknown app ids show an error view.
class AppView : public SiteView {
public:
    explicit AppView(SPElement* parent);
    AppView(SPElement* parent, bool showNavBar);
    bool   acceptsUri(const string& uri) override;
    void   navigate(const string& uri)   override;
    void   navigateBack(const string& uri) override;
    void   navigateForward(const string& uri) override;
    string uriToUrl(const string& uri)   const override;
    string urlToUri(const string& url)   const override;
    // Native apps restart with a fresh Lua state and reloaded routes.
    void   reload() override;

#ifdef BUNGALOW_CEF
    void setVisible(bool value) override;
    void paint(int x, int y, SPGraphicsContext* c) override;
    void showInspector() override;
#endif

private:
    SPButtonElement* m_addAppButton = nullptr;
    bool m_devMode = false;

    SPElement*     m_errorView = nullptr;
    SPTextElement* m_errorText = nullptr;

    std::map<string, SPNativeAppElement*> m_nativeApps; // by app id; children of this view
    SPNativeAppElement* m_activeNativeApp = nullptr;

    void updateAppNavBar(const string& uri);
    int  onAddAppClicked(SPObject* sender, SPEventArgs* args);

    // Shows uri in an installed app bundle (or the error view). Returns false,
    // with no app content shown, when uri belongs to the remote app path.
    bool navigateBundle(const string& uri);
    // Shows uri in a native app bundle, restarting the app first if asked.
    // Returns false if uri is not in a native app.
    bool navigateNativeApp(const string& uri, bool restart = false);
    SPNativeAppElement* nativeAppFor(const SPAppBundle& bundle, bool restart);
    // app is null to hide all app content (and the site browser).
    void showNativeApp(SPNativeAppElement* app);
    void showError(const string& message);

#ifdef BUNGALOW_CEF
    struct LocalApp {
        std::unique_ptr<SPEmbeddedBrowser> browser;
        string route; // in-app path last loaded or reported, e.g. "/a/b"
        string title;
    };
    std::map<string, LocalApp> m_localApps; // by app id
    LocalApp* m_activeApp = nullptr;

    SPButtonElement* m_devToolsButton = nullptr;

    // Shows uri in a local app browser or the error view. Returns false when
    // uri belongs to the remote (non-bundle) app path.
    bool navigateLocalApp(const string& uri);
    LocalApp* localAppFor(const SPAppBundle& bundle);
    void onRouteChanged(const string& appId, const string& path);
    void showLocalApp(LocalApp* app);
    int  onDevToolsClicked(SPObject* sender, SPEventArgs* args);
#endif
    static constexpr const char* kAppsDomain = ".apps.bungalow";
};

} // namespace views
} // namespace space
