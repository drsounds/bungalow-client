#include "SiteView.h"
#include "BungalowApp.h"
#include "SPWindowElement.h"
#include "SPViewStackElement.h"
#include "SPInputEventArgs.h"
#include "SPBookmarkEventArgs.h"
#include "DownloadManager.h"
#include "Database.h"

#include "SPString.h"

#include <Windows.h>
#include <shellapi.h>

namespace space {
namespace views {

SiteView* SiteView::s_activeInstance = nullptr;

static std::wstring widen(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(n > 0 ? n - 1 : 0, L'\0');
    if (n > 0) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), n);
    return w;
}

static bool isBungalowScheme(const string& uri) {
    return space::startsWith(uri, "bungalow:") ||
           space::startsWith(uri, "space:")    ||
           space::startsWith(uri, "bungalow:") ||
           space::startsWith(uri, "musik:");
}

string SiteView::uriToUrl(const string& uri) const {
    if (uri.size() <= m_uriPrefix.size()) return "";
    string remainder = uri.substr(m_uriPrefix.size());
    string url = "https://";
    for (char c : remainder)
        url += (c == ':') ? '/' : c;
    return url;
}

string SiteView::urlToUri(const string& url) const {
    const string scheme = "https://";
    if (url.rfind(scheme, 0) != 0) return "";
    string rest = url.substr(scheme.size());
    string uri = m_uriPrefix;
    for (char c : rest)
        uri += (c == '/') ? ':' : c;
    while (!uri.empty() && uri.back() == ':')
        uri.pop_back();
    return uri;
}

string SiteView::normalizeUrl(const string& url) {
    if (!url.empty() && url.back() == '/')
        return url.substr(0, url.size() - 1);
    return url;
}

string SiteView::extractHostname(const string& url) {
    size_t start = url.find("://");
    if (start == string::npos) return url;
    start += 3;
    size_t end = url.find_first_of("/?#", start);
    string hostPort = url.substr(start, end == string::npos ? string::npos : end - start);
    size_t colon = hostPort.rfind(':');
    if (colon != string::npos) hostPort = hostPort.substr(0, colon);
    return hostPort;
}

bool SiteView::isDomainAllowed(const string& url) const {
    string hostname = extractHostname(url);
    if (hostname.empty()) return true;

    if (hostname == "bungalow.se" || space::endsWith(hostname, ".bungalow.se"))
        return true;

    auto* app = BungalowApp::current();
    if (app && app->getDatabase())
        return app->getDatabase()->isDomainAccepted(hostname);

    return false;
}

bool SiteView::showThirdPartyWarning(const string& hostname) {
    std::wstring message =
        L"The site \"" + widen(hostname) + L"\" is a third party outside "
        L"of the Bungalow developer's responsibility.\n\n"
        L"The developers of Bungalow, including but not limited to its contributors, take no "
        L"responsibility for the content, data, or financial transactions on that site and "
        L"cannot be held liable for any losses caused by using this third-party website.\n\n"
        L"If you agree, click Yes to proceed. If you do not agree, click No and you "
        L"will be returned to the previous view.";

    if (MessageBoxW(GetActiveWindow(), message.c_str(), L"Third-Party Website",
                    MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING) != IDYES)
        return false;

    auto* app = BungalowApp::current();
    if (app && app->getDatabase())
        app->getDatabase()->addAcceptedDomain(hostname);
    return true;
}

SiteView::SiteView(SPElement* parent)
    : SiteView(parent, "bungalow:site:", true, true) {}

SiteView::SiteView(SPElement* parent, const string& uriPrefix, bool showNavBar, bool showUrlBar, bool showBookmarkButton)
    : SPViewElement(parent)
    , m_uriPrefix(uriPrefix)
    , m_navOffset(showNavBar ? kNavBarHeight : 0)
{
    this->clipView = false;

    if (showNavBar) {
        m_navRow = new SPHBoxElement(this);
        m_navRow->setHeight(kNavBarHeight);
        m_navRow->setPadding({ 4, 4, 4, 4 });
        m_navRow->setGap(6);
        this->appendChild(m_navRow);

        if (showBookmarkButton) {
            m_bookmarkButton = new SPButtonElement(m_navRow);
            m_bookmarkButton->setInnerText("\xe2\x98\x86");
            m_bookmarkButton->setWidth(kNavBarHeight - 8);
            m_bookmarkButton->setHeight(kNavBarHeight - 8);
            m_bookmarkButton->addEventListener("click",
                reinterpret_cast<SPCallback>(&SiteView::onBookmarkClicked), this, m_bookmarkButton);
            m_navRow->appendChild(m_bookmarkButton);
        }

        if (showUrlBar) {
            m_urlInput = new SPInputElement(m_navRow);
            m_urlInput->setFlex(1);
            m_urlInput->setHeight(kNavBarHeight - 8);
            m_urlInput->addEventListener("enter",
                reinterpret_cast<SPCallback>(&SiteView::onUrlEntered), this, m_urlInput);
            m_navRow->appendChild(m_urlInput);

            m_goButton = new SPButtonElement(m_navRow);
            m_goButton->setInnerText("Go");
            m_goButton->setWidth(48);
            m_goButton->setHeight(kNavBarHeight - 8);
            m_goButton->addEventListener("click",
                reinterpret_cast<SPCallback>(&SiteView::onGoClicked), this, m_goButton);
            m_navRow->appendChild(m_goButton);
        }
    }
}

SiteView::~SiteView() {
    if (s_activeInstance == this) s_activeInstance = nullptr;
}

void SiteView::runScript(const std::string& js) {
#ifdef BUNGALOW_CEF
    if (m_browser) m_browser->executeScript(js);
#else
    (void)js;
#endif
}

string SiteView::themeScript() {
    const string bgHex = getBackgroundColor().toHTMLColor();
    const string fgHex = getForegroundColor().toHTMLColor();
    const string scheme = (getColorScheme() == SP_COLOR_SCHEME_DARK) ? "dark" : "light";
    return string("(function(){") +
        "var e=document.getElementById('sp-theme');" +
        "if(!e){e=document.createElement('style');e.id='sp-theme';" +
        "(document.head||document.documentElement).appendChild(e);}" +
        "e.textContent='" +
        ":root{--sp-bg-color:" + bgHex + ";--sp-fg-color:" + fgHex + ";--sp-color-scheme:" + scheme + "}" +
        "body{background-color:" + bgHex + ";color:" + fgHex + "}" +
        "';})();";
}

void SiteView::setNavBarVisible(bool visible) {
    if (!m_navRow) return;
    m_navOffset = visible ? kNavBarHeight : 0;
    m_navRow->setVisible(visible);
}

void SiteView::updateUrlBar(const string& url) {
    if (m_urlInput) m_urlInput->setValue(url);
    string uri = urlToUri(url);
    if (!uri.empty()) updateBookmarkButton(uri);
}

void SiteView::updateBookmarkButton(const string& uri) {
    if (!m_bookmarkButton) return;
    auto* app = BungalowApp::current();
    if (!app || !app->getDatabase()) return;
    bool bookmarked = app->getDatabase()->isBookmarked(uri);
    m_bookmarkButton->setInnerText(bookmarked ? "\xe2\x98\x85" : "\xe2\x98\x86");
    m_bookmarkButton->invalidate();
}

bool SiteView::acceptsUri(const string& uri) {
    return space::startsWith(uri, "bungalow:site:");
}

void SiteView::navigate(const string& uri) {
    m_previousUri = m_currentUri;
    m_currentUri = uri;
    updateBookmarkButton(uri);

    string url = uriToUrl(uri);
    if (url.empty()) return;

    updateUrlBar(url);

#ifdef BUNGALOW_CEF
    // Already showing it, e.g. the page navigated itself and reported it.
    if (m_browser && normalizeUrl(url) == normalizeUrl(m_loadedUrl)) {
        m_showBrowser = true;
        invalidate();
        return;
    }
#endif

    if (auto* app = BungalowApp::current()) {
        if (auto* dm = app->getDownloadManager()) {
            m_pendingUrl  = url;
            m_checkingUrl = true;
            dm->checkUrl(url, [this](const string& checkedUrl,
                                     bool           isDownload,
                                     const string&  filename,
                                     const string&  contentType) {
                if (!m_checkingUrl || checkedUrl != m_pendingUrl) return;
                m_checkingUrl = false;
                m_pendingUrl.clear();

                if (isDownload) {
                    if (auto* a = BungalowApp::current())
                        if (auto* d = a->getDownloadManager())
                            d->startDownload(checkedUrl, filename, contentType);
                    getWindowElement()->navigate("bungalow:downloads");
                } else {
                    loadUrl(checkedUrl);
                }
            });
            return;
        }
    }

    loadUrl(url);
}

void SiteView::loadUrl(const string& url) {
    if (!isDomainAllowed(url)) {
        if (!showThirdPartyWarning(extractHostname(url))) {
            m_currentUri = m_previousUri;
            if (!m_previousUri.empty()) {
                updateUrlBar(uriToUrl(m_previousUri));
                updateBookmarkButton(m_previousUri);
            }
            if (auto* vs = dynamic_cast<SPViewStackElement*>(this->getParentElement()))
                if (vs->canGoBack()) vs->goBack();
            return;
        }
    }

    if (normalizeUrl(m_loadedUrl) == normalizeUrl(url)) {
#ifdef BUNGALOW_CEF
        if (m_browser) {
            m_showBrowser = true;
            invalidate();
        }
#endif
        return;
    }
    showUrl(url);
}

void SiteView::showUrl(const string& url) {
    m_loadedUrl = url;
#ifdef BUNGALOW_CEF
    if (auto* browser = ensureBrowser()) {
        m_loadingUrl = true;
        m_showBrowser = true;
        m_pageTitle.clear();
        browser->loadUrl(url);
        invalidate(); // the browser is positioned on the next paint
        return;
    }
#endif
    ShellExecuteW(nullptr, L"open", widen(url).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void SiteView::openExternalUrl(const string& url) {
    if (!isDomainAllowed(url) && !showThirdPartyWarning(extractHostname(url))) return;
    ShellExecuteW(nullptr, L"open", widen(url).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

// History entries were consented to when first visited, so they are shown
// without asking again or pushing a new entry. The system browser fallback
// only updates the URL bar.
void SiteView::navigateBack(const string& uri) {
    string url = uriToUrl(uri);
    if (url.empty()) return;
    m_currentUri = uri;
    updateUrlBar(url);
#ifdef BUNGALOW_CEF
    if (!ensureBrowser()) return;
    if (normalizeUrl(url) != normalizeUrl(m_loadedUrl)) {
        showUrl(url);
    } else {
        m_showBrowser = true;
        invalidate();
    }
#endif
}

void SiteView::navigateForward(const string& uri) {
    navigateBack(uri);
}

void SiteView::setVisible(bool value) {
    SPViewElement::setVisible(value);
    if (value)
        s_activeInstance = this;
    else if (s_activeInstance == this)
        s_activeInstance = nullptr;

#ifdef BUNGALOW_CEF
    if (!m_browser) return;
    if (value) {
        // Pick up theme changes made while hidden (e.g. in the settings view).
        m_browser->setDarkColorScheme(getColorScheme() == SP_COLOR_SCHEME_DARK);
        m_browser->executeScript(themeScript());
        invalidate();
    } else {
        m_browser->setVisible(false); // hidden views are not painted
    }
#endif
}

#ifdef BUNGALOW_CEF
// ── Embedded browser ──────────────────────────────────────────────────────────

SPEmbeddedSiteBrowser* SiteView::ensureBrowser() {
    if (m_browser || m_browserFailed) return m_browser.get();

    SPEmbeddedSiteBrowser::Options options;
    if (auto* app = BungalowApp::current())
        options.developerMode = app->isDeveloperMode();
    options.darkColorScheme = getColorScheme() == SP_COLOR_SCHEME_DARK;
    // Run the theme as soon as the document element exists, before page scripts.
    options.startupScript =
        "(function(apply){if(document.documentElement){apply();return;}"
        "new MutationObserver(function(_,o){if(document.documentElement){o.disconnect();apply();}})"
        ".observe(document,{childList:true});})(function(){" + themeScript() + "});";

    SPEmbeddedSiteBrowser::Callbacks callbacks;
    callbacks.addressChanged = [this](const string& url) { onAddressChanged(url); };
    callbacks.navigateRequested = [this](const string& uri) {
        if (isVisible()) getWindowElement()->navigate(uri);
    };
    callbacks.navigationRequested = [this](const string& url) { onNavigationRequested(url); };
    callbacks.downloadRequested = [this](const string& url) { onDownloadRequested(url); };
    callbacks.titleChanged = [this](const string& title) { m_pageTitle = title; };

    m_browser = createEmbeddedSiteBrowser(this, std::move(options), std::move(callbacks));
    m_browserFailed = !m_browser;
    return m_browser.get();
}

void SiteView::hideSiteBrowser() {
    m_showBrowser = false;
    if (m_browser) m_browser->setVisible(false);
}

// The page committed a new address: a load we started, a link the page
// followed on an allowed host, or a pushState/hash change.
void SiteView::onAddressChanged(const string& url) {
    const bool startedHere = m_loadingUrl;
    m_loadingUrl = false;
    m_loadedUrl = url;
    updateUrlBar(url);

    const string uri = urlToUri(url);
    if (uri.empty() || uri == m_currentUri) return;
    if (startedHere || !isVisible()) {
        // A redirect of our own load: no extra history entry.
        m_currentUri = uri;
        return;
    }
    // navigate() finds m_loadedUrl already showing and only records it.
    getWindowElement()->navigate(uri);
}

// The page tried to open a page on a host that has not been allowed yet (or in
// a new window): go through navigate() for consent and download detection.
void SiteView::onNavigationRequested(const string& url) {
    if (!isVisible()) return;
    const string uri = urlToUri(url);
    if (!uri.empty()) {
        getWindowElement()->navigate(uri);
        return;
    }
    // Plain http has no bungalow: URI; show it in place without history.
    if (isDomainAllowed(url) || showThirdPartyWarning(extractHostname(url)))
        showUrl(url);
}

void SiteView::onDownloadRequested(const string& url) {
    auto* app = BungalowApp::current();
    if (!app || !app->getDownloadManager()) return;
    app->getDownloadManager()->checkUrl(url, [this](const string& checkedUrl,
                                                    bool          /*isDownload*/,
                                                    const string& filename,
                                                    const string& contentType) {
        if (auto* a = BungalowApp::current())
            if (auto* d = a->getDownloadManager())
                d->startDownload(checkedUrl, filename, contentType);
        getWindowElement()->navigate("bungalow:downloads");
    });
}

// Absolute bounds are only known once painted, so the native browser is
// positioned and shown from here; layout changes and window resizes both end
// in a repaint.
void SiteView::paint(int x, int y, SPGraphicsContext* c) {
    SPViewElement::paint(x, y, c);
    if (!m_browser || !m_showBrowser || !isVisible()) return;
    const auto bounds = getAbsoluteBounds();
    m_browser->setBounds(bounds.x, bounds.y + m_navOffset, bounds.width, bounds.height - m_navOffset);
    m_browser->setVisible(true);
}
#endif

int SiteView::onBookmarkClicked(SPObject*, SPEventArgs*) {
    if (m_currentUri.empty()) return 0;
    const string& title = m_pageTitle.empty() ? m_currentUri : m_pageTitle;
    auto* args = new space::SPBookmarkEventArgs(m_currentUri, title);
    this->getWindowElement()->notify("bookmarktoggled", this, args);
    return 0;
}

int SiteView::onUrlEntered(SPObject*, SPEventArgs* args) {
    auto* ie = dynamic_cast<SPInputEventArgs*>(args);
    if (ie) navigateToInputUrl(ie->getText());
    return 0;
}

int SiteView::onGoClicked(SPObject*, SPEventArgs*) {
    if (m_urlInput) navigateToInputUrl(m_urlInput->getValue());
    return 0;
}

void SiteView::navigateToInputUrl(const string& text) {
    if (text.empty()) return;
    string resolved = text;
    if (resolved.find("://") == string::npos && !isBungalowScheme(resolved))
        resolved = "https://" + resolved;
    string bungalowUri;
    if (space::startsWith(resolved, "https://"))
        bungalowUri = urlToUri(resolved);
    else if (isBungalowScheme(resolved))
        bungalowUri = resolved;
    if (!bungalowUri.empty())
        this->getWindowElement()->navigate(bungalowUri);
}

void SiteView::reload() {
#ifdef BUNGALOW_CEF
    if (m_browser) {
        if (m_showBrowser) m_browser->reload();
        return;
    }
#endif
    if (!m_loadedUrl.empty())
        ShellExecuteW(nullptr, L"open", widen(m_loadedUrl).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void SiteView::showInspector() {
#ifdef BUNGALOW_CEF
    if (m_browser && m_showBrowser) m_browser->showDevTools();
#endif
}

} // namespace views
} // namespace space
