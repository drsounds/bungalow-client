#include "SiteView.h"
#include "SpacifyApp.h"
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

static bool isSpacifyScheme(const string& uri) {
    return space::startsWith(uri, "spacify:") ||
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

    if (hostname == "spacify.se" || space::endsWith(hostname, ".spacify.se"))
        return true;

    auto* app = SpacifyApp::current();
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

    auto* app = SpacifyApp::current();
    if (app && app->getDatabase())
        app->getDatabase()->addAcceptedDomain(hostname);
    return true;
}

SiteView::SiteView(SPElement* parent)
    : SiteView(parent, "spacify:site:", true, true) {}

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

void SiteView::runScript(const std::string&) {}

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
    auto* app = SpacifyApp::current();
    if (!app || !app->getDatabase()) return;
    bool bookmarked = app->getDatabase()->isBookmarked(uri);
    m_bookmarkButton->setInnerText(bookmarked ? "\xe2\x98\x85" : "\xe2\x98\x86");
    m_bookmarkButton->invalidate();
}

bool SiteView::acceptsUri(const string& uri) {
    return space::startsWith(uri, "spacify:site:");
}

void SiteView::navigate(const string& uri) {
    m_previousUri = m_currentUri;
    m_currentUri = uri;
    updateBookmarkButton(uri);

    string url = uriToUrl(uri);
    if (url.empty()) return;

    updateUrlBar(url);

    if (auto* app = SpacifyApp::current()) {
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
                    if (auto* a = SpacifyApp::current())
                        if (auto* d = a->getDownloadManager())
                            d->startDownload(checkedUrl, filename, contentType);
                    getWindowElement()->navigate("spacify:downloads");
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

    if (normalizeUrl(m_loadedUrl) == normalizeUrl(url)) return;
    m_loadedUrl = url;
    ShellExecuteW(nullptr, L"open", widen(url).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void SiteView::openExternalUrl(const string& url) {
    if (!isDomainAllowed(url) && !showThirdPartyWarning(extractHostname(url))) return;
    ShellExecuteW(nullptr, L"open", widen(url).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void SiteView::navigateBack(const string& uri) {
    string url = uriToUrl(uri);
    if (!url.empty()) {
        m_currentUri = uri;
        updateUrlBar(url);
    }
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
}

int SiteView::onBookmarkClicked(SPObject*, SPEventArgs*) {
    if (m_currentUri.empty()) return 0;
    auto* args = new space::SPBookmarkEventArgs(m_currentUri, m_currentUri);
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
    if (resolved.find("://") == string::npos && !isSpacifyScheme(resolved))
        resolved = "https://" + resolved;
    string spacifyUri;
    if (space::startsWith(resolved, "https://"))
        spacifyUri = urlToUri(resolved);
    else if (isSpacifyScheme(resolved))
        spacifyUri = resolved;
    if (!spacifyUri.empty())
        this->getWindowElement()->navigate(spacifyUri);
}

void SiteView::reload() {
    if (!m_loadedUrl.empty())
        ShellExecuteW(nullptr, L"open", widen(m_loadedUrl).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void SiteView::showInspector() {}

} // namespace views
} // namespace space
