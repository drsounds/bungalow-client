#pragma once
#include "SPViewElement.h"
#include "SPHBoxElement.h"
#include "SPInputElement.h"
#include "SPButtonElement.h"

#ifdef BUNGALOW_CEF
#include <memory>
#include "SPEmbeddedBrowser.h"
#endif

namespace space {
namespace views {

// Win32 counterpart of the wx SiteView. It keeps the Space-rendered navigation
// bar, URI mapping, third-party-domain consent and download detection.
//
// With BUNGALOW_CEF, pages are shown in an embedded browser below the nav bar;
// in-page navigations become view-stack history entries, and pages on hosts
// the user has not accepted yet go through the third-party consent first.
// Without it (or if the browser cannot start) pages open in the system browser.
class SiteView : public SPViewElement {
public:
    explicit SiteView(SPElement* parent);
    ~SiteView() override;

    bool acceptsUri(const string& uri) override;
    void navigate(const string& uri) override;
    void navigateBack(const string& uri) override;
    void navigateForward(const string& uri) override;
    void setVisible(bool value) override;
#ifdef BUNGALOW_CEF
    void paint(int x, int y, SPGraphicsContext* c) override;
#endif

    virtual void showInspector();
    virtual void reload();

    static SiteView* getActiveInstance() { return s_activeInstance; }

protected:
    SiteView(SPElement* parent, const string& uriPrefix, bool showNavBar, bool showUrlBar = true, bool showBookmarkButton = true);

    virtual string uriToUrl(const string& uri) const;
    virtual string urlToUri(const string& url) const;

    void updateBookmarkButton(const string& uri);
    void setNavBarVisible(bool visible);
    const string& getCurrentUri() const { return m_currentUri; }
    void setCurrentUri(const string& uri) { m_currentUri = uri; }
    int getNavOffset() const { return m_navOffset; }

    bool isDomainAllowed(const string& url) const;
    bool showThirdPartyWarning(const string& hostname);
    // Opens url in the system browser after the third-party consent check.
    void openExternalUrl(const string& url);

    // Runs js in the current page; a no-op without an embedded browser.
    void runScript(const std::string& js);

    // Exposes the app theme to pages as CSS variables and body colors.
    string themeScript();

    // Title of the current page, or empty.
    const string& getPageTitle() const { return m_pageTitle; }

#ifdef BUNGALOW_CEF
    // Hides the site browser until the next site navigation, for subclasses
    // that show other content in its place.
    void hideSiteBrowser();
#endif

    static constexpr int kNavBarHeight = 36;

    SPHBoxElement*   m_navRow        = nullptr;

private:
    string m_uriPrefix;
    int    m_navOffset;
    string m_currentUri;

    SPButtonElement* m_bookmarkButton = nullptr;
    SPInputElement*  m_urlInput      = nullptr;
    SPButtonElement* m_goButton      = nullptr;

    string m_pendingUrl;
    bool   m_checkingUrl = false;
    string m_previousUri;
    string m_loadedUrl;
    string m_pageTitle;

    void updateUrlBar(const string& url);
    void navigateToInputUrl(const string& text);
    void loadUrl(const string& url);
    // Shows url without the consent check or a new history entry.
    void showUrl(const string& url);

#ifdef BUNGALOW_CEF
    std::unique_ptr<SPEmbeddedSiteBrowser> m_browser;
    bool m_browserFailed = false; // fall back to the system browser
    bool m_showBrowser = false;   // the site browser is this view's content
    bool m_loadingUrl = false;    // a load we started has not committed yet

    SPEmbeddedSiteBrowser* ensureBrowser();
    void onAddressChanged(const string& url);
    void onNavigationRequested(const string& url);
    void onDownloadRequested(const string& url);
#endif

    int onBookmarkClicked(SPObject* sender, SPEventArgs* args);
    int onUrlEntered(SPObject* sender, SPEventArgs* args);
    int onGoClicked(SPObject* sender, SPEventArgs* args);

    static string normalizeUrl(const string& url);
    static string extractHostname(const string& url);

    static SiteView* s_activeInstance;
};

} // namespace views
} // namespace space
