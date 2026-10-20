#include "SiteView.h"
#include "BungalowApp.h"
#include "SPWindowElement.h"
#include "SPViewStackElement.h"
#include "SPInputEventArgs.h"
#include "SPBookmarkEventArgs.h"
#include "WxWidgetsGraphicsContext.h"
#include "SpaceWidget.h"
#include "DownloadManager.h"
#include "Database.h"

#include "SPString.h"
#include <wx/msgdlg.h>

namespace space {
namespace views {

SiteView* SiteView::s_activeInstance = nullptr;

// ── URI <-> URL translation ───────────────────────────────────────────────────

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

// Extract the hostname from an http(s):// URL (strips scheme, port, and path).
string SiteView::extractHostname(const string& url) {
    size_t start = url.find("://");
    if (start == string::npos) return url;
    start += 3;
    size_t end = url.find_first_of("/?#", start);
    string hostPort = url.substr(start, end == string::npos ? string::npos : end - start);
    // Strip optional :port suffix
    size_t colon = hostPort.rfind(':');
    if (colon != string::npos) hostPort = hostPort.substr(0, colon);
    return hostPort;
}

// Returns true if the URL's hostname is whitelisted (*.bungalow.se) or has been
// previously accepted by the user and recorded in the database.
bool SiteView::isDomainAllowed(const string& url) const {
    string hostname = extractHostname(url);
    if (hostname.empty()) return true;

    // Bungalow-operated domains are always allowed
    if (hostname == "bungalow.se" || space::endsWith(hostname, ".bungalow.se"))
        return true;

    auto* app = dynamic_cast<BungalowApp*>(wxTheApp);
    if (app && app->getDatabase())
        return app->getDatabase()->isDomainAccepted(hostname);

    return false;
}

// Shows the third-party warning dialog.  Returns true if the user clicked
// "Agree and Proceed", false if they clicked "Go Back".
// On acceptance the hostname is persisted to the database.
bool SiteView::showThirdPartyWarning(const string& hostname) {
    wxString message =
        "The site \"" + wxString::FromUTF8(hostname.c_str()) + "\" is a third party outside "
        "of the Bungalow developer's responsibility.\n\n"
        "The developers of Bungalow, including but not limited to its contributors, take no "
        "responsibility for the content, data, or financial transactions on that site and "
        "cannot be held liable for any losses caused by using this third-party website.\n\n"
        "If you agree, click Agree and Proceed. If you do not agree, click Go Back and you "
        "will be returned to the previous view.";

    wxMessageDialog dlg(nullptr, message, "Third-Party Website",
                        wxYES_NO | wxNO_DEFAULT | wxICON_WARNING);
    dlg.SetYesNoLabels("Agree and Proceed", "Go Back");

    if (dlg.ShowModal() != wxID_YES) return false;

    auto* app = dynamic_cast<BungalowApp*>(wxTheApp);
    if (app && app->getDatabase())
        app->getDatabase()->addAcceptedDomain(hostname);
    return true;
}

// ── Constructor / Destructor ──────────────────────────────────────────────────

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
            // Bookmark toggle button — present when nav bar is shown and not suppressed by subclass
            m_bookmarkButton = new SPButtonElement(m_navRow);
            m_bookmarkButton->setInnerText("\xe2\x98\x86"); // ☆
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
    if (m_webView) { m_webView->Destroy(); m_webView = nullptr; }
}

// ── Lazy webview creation ─────────────────────────────────────────────────────

void SiteView::ensureWebView() {
    if (m_webView) return;

    auto b = getAbsoluteBounds();
    if (b.width == 0 || b.height <= m_navOffset) return;

    auto* gc = dynamic_cast<platform::wxwidgets::WxWidgetsGraphicsContext*>(createGraphics());
    if (!gc) return;
    SpaceWidget* spaceWidget = gc->getWidget();
    if (!spaceWidget) return;

    // Sit below the Space-rendered nav row (m_navOffset = 0 when nav bar hidden)
    m_webView = wxWebView::New(spaceWidget, wxID_ANY, wxWebViewDefaultURLStr,
                               wxPoint(b.x, b.y + m_navOffset),
                               wxSize(b.width, b.height - m_navOffset));
    m_webView->Hide();

    bool devMode = false;
    if (auto* app = dynamic_cast<BungalowApp*>(wxTheApp))
        devMode = app->isDeveloperMode();

    // Inject app theme colors and expose CSS variables at document start
    m_webView->AddUserScript(buildThemeScript(), wxWEBVIEW_INJECT_AT_DOCUMENT_START);
    platformApplyColorScheme(getColorScheme());

    if (!devMode) {
        // Suppress right-click context menu when not in developer mode
        m_webView->AddUserScript(
            "document.addEventListener('contextmenu',function(e){e.preventDefault();},true);",
            wxWEBVIEW_INJECT_AT_DOCUMENT_START);
    }
    platformConfigureWebView(devMode);

    m_webView->Bind(wxEVT_WEBVIEW_NAVIGATING, &SiteView::onWebViewNavigating, this);
    m_webView->Bind(wxEVT_WEBVIEW_NAVIGATED,  &SiteView::onWebViewNavigated,  this);
    m_webView->Bind(wxEVT_WEBVIEW_ERROR,      &SiteView::onWebViewError,      this);

    if (m_webView->AddScriptMessageHandler("bungalowNav")) {
        m_webView->AddUserScript(R"js(
(function() {
    var _push    = history.pushState.bind(history);
    var _replace = history.replaceState.bind(history);
    function notify(url) {
        try { window.webkit.messageHandlers.bungalowNav.postMessage(url); } catch(e) {}
    }
    history.pushState = function(s, t, u) {
        _push(s, t, u);
        notify(window.location.href);
    };
    history.replaceState = function(s, t, u) {
        _replace(s, t, u);
        notify(window.location.href);
    };
    window.addEventListener('hashchange', function() {
        notify(window.location.href);
    });
})();
)js", wxWEBVIEW_INJECT_AT_DOCUMENT_START);
        m_webView->Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED,
                        &SiteView::onScriptMessage, this);
    }
}

void SiteView::positionWebView() {
    if (!m_webView) return;
    auto b = getAbsoluteBounds();
    m_webView->SetPosition(wxPoint(b.x, b.y + m_navOffset));
    m_webView->SetSize(wxSize(b.width, b.height - m_navOffset));
}

void SiteView::runScript(const std::string& js) {
    if (m_webView)
        m_webView->RunScript(wxString::FromUTF8(js));
}

void SiteView::hideSiteBrowser() {
    m_webViewHidden = true;
    if (m_webView) m_webView->Hide();
}

void SiteView::setNavBarVisible(bool visible) {
    if (!m_navRow) return;
    m_navOffset = visible ? kNavBarHeight : 0;
    m_navRow->setVisible(visible);
    positionWebView();
}

void SiteView::updateUrlBar(const string& url) {
    if (m_urlInput) m_urlInput->setValue(url);
    string uri = urlToUri(url);
    if (!uri.empty()) updateBookmarkButton(uri);
}

void SiteView::updateBookmarkButton(const string& uri) {
    if (!m_bookmarkButton) return;
    auto* app = dynamic_cast<BungalowApp*>(wxTheApp);
    if (!app || !app->getDatabase()) return;
    bool bookmarked = app->getDatabase()->isBookmarked(uri);
    m_bookmarkButton->setInnerText(bookmarked ? "\xe2\x98\x85" : "\xe2\x98\x86"); // ★ / ☆
    m_bookmarkButton->invalidate();
}

// ── SPViewElement interface ───────────────────────────────────────────────────

bool SiteView::acceptsUri(const string& uri) {
    return space::startsWith(uri, "bungalow:site:");
}

void SiteView::navigate(const string& uri) {
    m_previousUri = m_currentUri;
    m_currentUri = uri;
    m_webViewHidden = false;
    updateBookmarkButton(uri);

    string url = uriToUrl(uri);
    if (url.empty()) return;

    ensureWebView();
    updateUrlBar(url);

    // Ask the download manager whether this URL is a downloadable resource.
    // If there's no download manager, or the URL is clearly a web page, load immediately.
    if (auto* app = dynamic_cast<BungalowApp*>(wxTheApp)) {
        if (auto* dm = app->getDownloadManager()) {
            m_pendingUrl   = url;
            m_checkingUrl  = true;
            dm->checkUrl(url, [this](const string& checkedUrl,
                                     bool           isDownload,
                                     const string&  filename,
                                     const string&  contentType) {
                if (!m_checkingUrl || checkedUrl != m_pendingUrl) return;
                m_checkingUrl = false;
                m_pendingUrl.clear();

                if (isDownload) {
                    if (auto* a = dynamic_cast<BungalowApp*>(wxTheApp))
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
            // User declined — restore visual state to the previous URI
            m_currentUri = m_previousUri;
            if (!m_previousUri.empty()) {
                updateUrlBar(uriToUrl(m_previousUri));
                updateBookmarkButton(m_previousUri);
            }
            // Mirror the app's Back button behaviour by going back in the view stack
            if (auto* vs = dynamic_cast<SPViewStackElement*>(this->getParentElement()))
                if (vs->canGoBack()) vs->goBack();
            return;
        }
        // User accepted — domain is now recorded in the DB, fall through to load
    }

    if (!m_webView) {
        m_pendingUrl = url;
        return;
    }
    if (isVisible() && !m_webViewHidden) m_webView->Show();
    if (normalizeUrl(m_webView->GetCurrentURL().ToStdString()) == normalizeUrl(url)) return;
    m_navigatingProgrammatically = true;
    m_webView->LoadURL(url);
}

void SiteView::navigateBack(const string& uri) {
    m_webViewHidden = false;
    ensureWebView();
    if (!m_webView) return;
    if (isVisible()) m_webView->Show();
    m_navigatingProgrammatically = true;
    if (m_webView->CanGoBack()) {
        m_webView->GoBack();
    } else {
        string url = uriToUrl(uri);
        if (!url.empty() && normalizeUrl(m_webView->GetCurrentURL().ToStdString()) != normalizeUrl(url)) {
            updateUrlBar(url);
            m_webView->LoadURL(url);
        } else {
            m_navigatingProgrammatically = false;
        }
    }
}

void SiteView::navigateForward(const string& uri) {
    m_webViewHidden = false;
    ensureWebView();
    if (!m_webView) return;
    if (isVisible()) m_webView->Show();
    m_navigatingProgrammatically = true;
    if (m_webView->CanGoForward()) {
        m_webView->GoForward();
    } else {
        string url = uriToUrl(uri);
        if (!url.empty() && normalizeUrl(m_webView->GetCurrentURL().ToStdString()) != normalizeUrl(url)) {
            updateUrlBar(url);
            m_webView->LoadURL(url);
        } else {
            m_navigatingProgrammatically = false;
        }
    }
}

void SiteView::setVisible(bool value) {
    SPViewElement::setVisible(value);
    if (value)
        s_activeInstance = this;
    else if (s_activeInstance == this)
        s_activeInstance = nullptr;
    if (m_webView) {
        if (value && !m_webViewHidden) {
            positionWebView();
            m_webView->Show();
            applyThemeToWebView();
        } else {
            m_webView->Hide();
        }
    }
    if (m_urlInput) {
        auto* gc = dynamic_cast<platform::wxwidgets::WxWidgetsGraphicsContext*>(createGraphics());
        if (gc) gc->setControlVisible(m_urlInput, value);
    }
}

void SiteView::pack() {
    SPViewElement::pack();
    if (!m_webView) {
        ensureWebView();
        if (m_webView) {
            if (isVisible() && !m_webViewHidden) m_webView->Show();
            if (!m_pendingUrl.empty() && !m_checkingUrl) {
                string url = m_pendingUrl;
                m_pendingUrl.clear();
                updateUrlBar(url);
                m_navigatingProgrammatically = true;
                m_webView->LoadURL(url);
            }
        }
    } else {
        positionWebView();
    }
}

// ── Webview event handlers ────────────────────────────────────────────────────

// Returns true for any URI that uses the bungalow: scheme or an accepted alias.
static bool isBungalowScheme(const string& uri) {
    return space::startsWith(uri, "bungalow:") ||
           space::startsWith(uri, "space:")    ||
           space::startsWith(uri, "bungalow:") ||
           space::startsWith(uri, "musik:");
}

// Called BEFORE the webview loads a URL.
// Intercepts bungalow: scheme hrefs and forwards them to the global viewstack
// instead of letting the webview attempt to load an unknown protocol.
// Also intercepts user-initiated navigations to unaccepted third-party domains.
void SiteView::onWebViewNavigating(wxWebViewEvent& event) {
    if (!event.IsTargetMainFrame()) return;
    string url = event.GetURL().ToStdString();
    if (isBungalowScheme(url)) {
        event.Veto();
        if (!m_navigatingProgrammatically)
            this->getWindowElement()->navigate(url);
    } else if (!m_navigatingProgrammatically && !isDomainAllowed(url)) {
        event.Veto();
        if (showThirdPartyWarning(extractHostname(url)))
            loadUrl(url); // domain is now accepted — proceed
        // else: user declined, webview remains on current page
    }
}

void SiteView::onWebViewNavigated(wxWebViewEvent& event) {
    if (!event.IsTargetMainFrame()) return;
    string url = event.GetURL().ToStdString();
    updateUrlBar(url);
    string bungalowUri = urlToUri(url);
    if (!bungalowUri.empty()) {
        m_currentUri = bungalowUri;
        updateBookmarkButton(bungalowUri);
    }
    applyThemeToWebView();
    if (m_navigatingProgrammatically) {
        m_navigatingProgrammatically = false;
        return;
    }
    if (!bungalowUri.empty())
        this->getWindowElement()->navigate(bungalowUri);
}

void SiteView::onWebViewError(wxWebViewEvent& /*event*/) {
    m_navigatingProgrammatically = false;
}

void SiteView::onScriptMessage(wxWebViewEvent& event) {
    if (m_navigatingProgrammatically) return;
    string url = event.GetString().ToStdString();
    updateUrlBar(url);
    string bungalowUri = urlToUri(url);
    if (!bungalowUri.empty())
        this->getWindowElement()->navigate(bungalowUri);
}

// ── Nav bar handlers ──────────────────────────────────────────────────────────

int SiteView::onBookmarkClicked(SPObject* /*sender*/, SPEventArgs* /*args*/) {
    if (m_currentUri.empty()) return 0;
    string title = (m_webView && !m_webView->GetCurrentTitle().IsEmpty())
        ? m_webView->GetCurrentTitle().ToStdString()
        : m_currentUri;
    auto* args = new space::SPBookmarkEventArgs(m_currentUri, title);
    this->getWindowElement()->notify("bookmarktoggled", this, args);
    return 0;
}

int SiteView::onUrlEntered(SPObject* /*sender*/, SPEventArgs* args) {
    auto* ie = dynamic_cast<SPInputEventArgs*>(args);
    if (ie) navigateToInputUrl(ie->getText());
    return 0;
}

int SiteView::onGoClicked(SPObject* /*sender*/, SPEventArgs* /*args*/) {
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

// ── Developer tools ───────────────────────────────────────────────────────────

void SiteView::reload() {
    if (m_webView) m_webView->Reload();
}

#ifdef __WXMAC__
extern "C" void SiteView_macShowInspector(void*);
extern "C" void SiteView_macConfigureWebView(void*, bool);
extern "C" void SiteView_macApplyColorScheme(void*, int);
#endif

void SiteView::showInspector() {
#ifdef __WXMAC__
    if (!m_webView) return;
    SiteView_macShowInspector(m_webView->GetNativeBackend());
#endif
}

void SiteView::platformConfigureWebView(bool developerMode) {
#ifdef __WXMAC__
    if (!m_webView) return;
    SiteView_macConfigureWebView(m_webView->GetNativeBackend(), developerMode);
#endif
}

void SiteView::platformApplyColorScheme(short scheme) {
#ifdef __WXMAC__
    if (!m_webView) return;
    SiteView_macApplyColorScheme(m_webView->GetNativeBackend(), (int)scheme);
#endif
}

string SiteView::buildThemeScript() {
    string bgHex = getBackgroundColor().toHTMLColor();
    string fgHex = getForegroundColor().toHTMLColor();
    short scheme = getColorScheme();
    // SP_COLOR_SCHEME_DEFAULT (0): getBackgroundColor() returns light colors and
    // the macOS appearance is nil (follows OS), so --sp-color-scheme reports "light"
    // to match the injected body colors even if the OS is in dark mode.
    string schemeStr = (scheme == SP_COLOR_SCHEME_DARK) ? "dark" : "light";

    return string("(function(){") +
        "var e=document.getElementById('sp-theme');" +
        "if(!e){e=document.createElement('style');e.id='sp-theme';" +
        "(document.head||document.documentElement).appendChild(e);}" +
        "e.textContent='" +
        ":root{--sp-bg-color:" + bgHex + ";--sp-fg-color:" + fgHex + ";--sp-color-scheme:" + schemeStr + "}" +
        "body{background-color:" + bgHex + ";color:" + fgHex + "}" +
        "';})();";
}

void SiteView::applyThemeToWebView() {
    if (!m_webView) return;
    m_webView->RunScript(buildThemeScript());
    platformApplyColorScheme(getColorScheme());
}

} // namespace views
} // namespace space
