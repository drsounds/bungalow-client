#include "AppView.h"
#include "SpacifyApp.h"
#include "SPViewElement.h"
#include "SPWindowElement.h"
#include "SPButtonElement.h"
#include "SPEventArgs.h"
#include "Database.h"
#include "SPBookmarkEventArgs.h"
#include "Database.h"
#include <boost/algorithm/string/predicate.hpp>

namespace space {
namespace views {

static bool getDevMode() {
    if (auto* app = dynamic_cast<SpacifyApp*>(wxTheApp))
        return app->isDeveloperMode();
    return false;
}

AppView::AppView(SPElement* parent)
    : SiteView(parent, "spacify:app:", true, getDevMode(), false) // no bookmark button — we add "Add app" instead
    , m_devMode(getDevMode())
{
    if (!m_navRow) return;

    if (!m_devMode) {
        // Non-dev mode: flexible spacer pushes "Add app" to the right
        auto* spacer = new SPHBoxElement(m_navRow);
        spacer->setFlex(1);
        m_navRow->appendChild(spacer);
    }

    m_addAppButton = new SPButtonElement(m_navRow);
    m_addAppButton->setInnerText("Add app");
    m_addAppButton->setHeight(kNavBarHeight - 8);
    m_addAppButton->addEventListener("click",
        reinterpret_cast<SPCallback>(&AppView::onAddAppClicked), this, m_addAppButton);
    m_navRow->appendChild(m_addAppButton);
}

AppView::AppView(SPElement* parent, bool showNavBar)
    : AppView(parent)
{
    if (!showNavBar) setNavBarVisible(false);
}

bool AppView::acceptsUri(const string& uri) {
    return boost::algorithm::starts_with(uri, "spacify:app:");
}

void AppView::navigate(const string& uri) {
    SiteView::navigate(uri);
    if (!m_devMode)
        updateAppNavBar(uri);
}

void AppView::updateAppNavBar(const string& uri) {
    auto* app = dynamic_cast<SpacifyApp*>(wxTheApp);
    if (!app || !app->getDatabase()) return;
    bool favorited = app->getDatabase()->isBookmarked(uri);
    setNavBarVisible(!favorited);
}

int AppView::onAddAppClicked(SPObject* /*sender*/, SPEventArgs* /*args*/) {
    const string& uri = getCurrentUri();
    if (uri.empty()) return 0;
    string title = uri; // fallback; webview title not directly accessible here
    auto* args = new space::SPBookmarkEventArgs(uri, title);
    this->getWindowElement()->notify("bookmarktoggled", this, args);
    setNavBarVisible(false);
    return 0;
}

// spacify:app:myapp        -> https://myapp.apps.spacify.se
// spacify:app:example.com  -> https://example.com
// spacify:app:myapp:page   -> https://myapp.apps.spacify.se/page
string AppView::uriToUrl(const string& uri) const {
    const string prefix = "spacify:app:";
    if (uri.size() <= prefix.size()) return "";

    string remainder = uri.substr(prefix.size());
    size_t colon     = remainder.find(':');
    string domain    = (colon == string::npos) ? remainder : remainder.substr(0, colon);
    string rest      = (colon == string::npos) ? "" : remainder.substr(colon);

    if (domain.find('.') == string::npos && !domain.empty())
        domain += kAppsDomain;

    string url = "https://" + domain;
    for (char c : rest)
        url += (c == ':') ? '/' : c;
    return url;
}

// https://myapp.apps.spacify.se/page -> spacify:app:myapp:page
// https://example.com/page           -> spacify:app:example.com:page
string AppView::urlToUri(const string& url) const {
    const string scheme = "https://";
    if (url.rfind(scheme, 0) != 0) return "";

    string rest      = url.substr(scheme.size());
    size_t slash     = rest.find('/');
    string domain    = (slash == string::npos) ? rest : rest.substr(0, slash);
    string path      = (slash == string::npos) ? "" : rest.substr(slash);

    // Collapse .apps.spacify.se back to the bare app name
    const string suffix = kAppsDomain;
    if (domain.size() > suffix.size() &&
        domain.substr(domain.size() - suffix.size()) == suffix)
        domain = domain.substr(0, domain.size() - suffix.size());

    string uri = "spacify:app:" + domain;
    for (char c : path)
        uri += (c == '/') ? ':' : c;
    while (!uri.empty() && uri.back() == ':')
        uri.pop_back();
    return uri;
}

} // namespace views
} // namespace space
