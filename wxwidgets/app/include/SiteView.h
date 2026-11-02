#pragma once
#include "SPViewElement.h"
#include "SPHBoxElement.h"
#include "SPInputElement.h"
#include "SPButtonElement.h"
#include <wx/webview.h>
#include <wx/webrequest.h>

namespace space {
namespace views {

class SiteView : public SPViewElement {
public:
    explicit SiteView(SPElement* parent);
    ~SiteView() override;

    bool acceptsUri(const string& uri) override;
    void navigate(const string& uri) override;
    void navigateBack(const string& uri) override;
    void navigateForward(const string& uri) override;
    void setVisible(bool value) override;
    void pack() override;

    void showInspector();
    virtual void reload();

    static SiteView* getActiveInstance() { return s_activeInstance; }

protected:
    // For subclasses: customise URI scheme, suppress URL bar, or show bookmark-only bar.
    // showNavBar=false: no nav row at all (legacy no-bar mode)
    // showNavBar=true, showUrlBar=true: full bar (URL input + Go + bookmark)
    // showNavBar=true, showUrlBar=false: minimal bar (bookmark button only)
    // showNavBar=true, showBookmarkButton=false: nav row exists but no bookmark button (subclass adds its own)
    SiteView(SPElement* parent, const string& uriPrefix, bool showNavBar, bool showUrlBar = true, bool showBookmarkButton = true);

    virtual string uriToUrl(const string& uri) const;
    virtual string urlToUri(const string& url) const;

    void updateBookmarkButton(const string& uri);
    void setNavBarVisible(bool visible);
    const string& getCurrentUri() const { return m_currentUri; }
    void setCurrentUri(const string& uri) { m_currentUri = uri; }

    /// Hides the web view until the next site navigation, for subclasses
    /// that show other content in its place.
    void hideSiteBrowser();

    /// Execute a JavaScript snippet in the embedded web view.
    /// No-ops if the web view does not exist yet.
    void runScript(const std::string& js);

    static constexpr int kNavBarHeight = 36;

    // Nav bar — Space framework elements rendered on the canvas (null when hidden)
    SPHBoxElement*   m_navRow        = nullptr;

private:
    string m_uriPrefix;   // e.g. "bungalow:site:" or "bungalow:app:"
    int    m_navOffset;   // kNavBarHeight when nav bar shown, else 0
    string m_currentUri;  // bungalow URI currently loaded

    SPButtonElement* m_bookmarkButton = nullptr;
    SPInputElement*  m_urlInput      = nullptr;
    SPButtonElement* m_goButton      = nullptr;

    // Webview — native overlay positioned below the nav row
    wxWebView* m_webView = nullptr;
    bool m_webViewHidden = false; // see hideSiteBrowser()

    bool   m_navigatingProgrammatically = false;
    string m_pendingUrl;
    bool   m_checkingUrl = false;
    string m_previousUri; // URI active before the most recent navigate() call

    void ensureWebView();
    void positionWebView();
    void updateUrlBar(const string& url);
    void navigateToInputUrl(const string& text);
    void loadUrl(const string& url);

    int onBookmarkClicked(SPObject* sender, SPEventArgs* args);
    int onUrlEntered(SPObject* sender, SPEventArgs* args);
    int onGoClicked(SPObject* sender, SPEventArgs* args);

    void onWebViewNavigating(wxWebViewEvent& event);
    void onWebViewNavigated(wxWebViewEvent& event);
    void onWebViewError(wxWebViewEvent& event);
    void onScriptMessage(wxWebViewEvent& event);

    static string normalizeUrl(const string& url);
    static string extractHostname(const string& url);
    bool isDomainAllowed(const string& url) const;
    bool showThirdPartyWarning(const string& hostname);
    void platformConfigureWebView(bool developerMode);
    void platformApplyColorScheme(short scheme);

    string buildThemeScript();
    void applyThemeToWebView();

    static SiteView* s_activeInstance;
};

} // namespace views
} // namespace space
