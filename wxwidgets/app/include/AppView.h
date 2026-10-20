#pragma once
#include <SiteView.h>

namespace space {
namespace views {

// Like SiteView but handles bungalow:app: URIs.
// Dev mode: full URL bar always shown.
// Non-dev mode: nav bar with "Add app" button shown only while the app is not in favourites.
class AppView : public SiteView {
public:
    explicit AppView(SPElement* parent);
    AppView(SPElement* parent, bool showNavBar);
    bool   acceptsUri(const string& uri) override;
    void   navigate(const string& uri)   override;
    string uriToUrl(const string& uri)   const override;
    string urlToUri(const string& url)   const override;

private:
    SPButtonElement* m_addAppButton = nullptr;
    bool m_devMode = false;

    void updateAppNavBar(const string& uri);
    int  onAddAppClicked(SPObject* sender, SPEventArgs* args);

    static constexpr const char* kAppsDomain = ".apps.bungalow.se";
};

} // namespace views
} // namespace space
