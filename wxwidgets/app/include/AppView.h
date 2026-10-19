#pragma once
#include <SiteView.h>

#ifdef BUNGALOW_CEF
#include <map>
#include <memory>
#include "SPEmbeddedBrowser.h"
#endif

namespace space {
#ifdef BUNGALOW_CEF
struct SPAppBundle;
class SPTextElement;
#endif

namespace views {

// Like SiteView but handles spacify:app: URIs.
// Dev mode: full URL bar always shown.
// Non-dev mode: nav bar with "Add app" button shown only while the app is not in favourites.
//
// With BUNGALOW_CEF, installed bundles that contain an index.html run locally
// in an embedded browser (one per app, kept alive while switching), served
// from sp://<app_id>/. spacify:app:<app_id>:a:b maps to the in-app route /a/b.
// Unknown app ids show an error view.
class AppView : public SiteView {
public:
    explicit AppView(SPElement* parent);
    AppView(SPElement* parent, bool showNavBar);
    bool   acceptsUri(const string& uri) override;
    void   navigate(const string& uri)   override;
    string uriToUrl(const string& uri)   const override;
    string urlToUri(const string& url)   const override;

#ifdef BUNGALOW_CEF
    void navigateBack(const string& uri) override;
    void navigateForward(const string& uri) override;
    void setVisible(bool value) override;
    void paint(int x, int y, SPGraphicsContext* c) override;
    void showInspector() override;
    void reload() override;
#endif

private:
    SPButtonElement* m_addAppButton = nullptr;
    bool m_devMode = false;

    void updateAppNavBar(const string& uri);
    int  onAddAppClicked(SPObject* sender, SPEventArgs* args);

    static constexpr const char* kAppsDomain = ".apps.spacify.se";

#ifdef BUNGALOW_CEF
    struct LocalApp {
        std::unique_ptr<SPEmbeddedBrowser> browser;
        string route; // in-app path last loaded or reported, e.g. "/a/b"
        string title;
    };
    std::map<string, LocalApp> m_localApps; // by app id
    LocalApp* m_activeApp = nullptr;

    SPElement*       m_errorView = nullptr;
    SPTextElement*   m_errorText = nullptr;
    SPButtonElement* m_devToolsButton = nullptr;

    // Shows uri in a local app browser or the error view. Returns false when
    // uri belongs to the remote (non-bundle) app path.
    bool navigateLocalApp(const string& uri);
    LocalApp* localAppFor(const SPAppBundle& bundle);
    void onRouteChanged(const string& appId, const string& path);
    void showLocalApp(LocalApp* app);
    void showError(const string& message);
    int  onDevToolsClicked(SPObject* sender, SPEventArgs* args);
#endif
};

} // namespace views
} // namespace space
