#pragma once
#include "SPViewElement.h"
#include "SPHBoxElement.h"
#include "SPInputElement.h"
#include "SPButtonElement.h"

namespace space {
namespace views {

// Win32 counterpart of the wx SiteView. It keeps the Space-rendered navigation
// bar, URI mapping, third-party-domain consent and download detection. Pages are
// opened in the system browser because no embedded web view is available.
class SiteView : public SPViewElement {
public:
    explicit SiteView(SPElement* parent);
    ~SiteView() override;

    bool acceptsUri(const string& uri) override;
    void navigate(const string& uri) override;
    void navigateBack(const string& uri) override;
    void navigateForward(const string& uri) override;
    void setVisible(bool value) override;

    void showInspector();
    void reload();

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

    // No embedded web view on this backend; kept for API parity.
    void runScript(const std::string& js);

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

    void updateUrlBar(const string& url);
    void navigateToInputUrl(const string& text);
    void loadUrl(const string& url);

    int onBookmarkClicked(SPObject* sender, SPEventArgs* args);
    int onUrlEntered(SPObject* sender, SPEventArgs* args);
    int onGoClicked(SPObject* sender, SPEventArgs* args);

    static string normalizeUrl(const string& url);
    static string extractHostname(const string& url);

    static SiteView* s_activeInstance;
};

} // namespace views
} // namespace space
