#include "AppView.h"
#include "SpacifyApp.h"
#include "SPViewElement.h"
#include "SPWindowElement.h"
#include "SPButtonElement.h"
#include "SPEventArgs.h"
#include "Database.h"
#include "SPBookmarkEventArgs.h"
#include "Database.h"
#include "SPString.h"

#ifdef BUNGALOW_CEF
#include "SPAppManager.h"
#include "SPTextElement.h"
#include "SPVBoxElement.h"

#include <filesystem>
#endif

namespace space {
namespace views {

static bool getDevMode() {
    if (auto* app = SpacifyApp::current())
        return app->isDeveloperMode();
    return false;
}

#ifdef BUNGALOW_CEF
namespace {

constexpr const char* kAppPrefix = "spacify:app:";

// spacify:app:<id>:a:b -> id "<id>", path "/a/b"
bool splitAppUri(const string& uri, string& appId, string& path) {
    if (!space::startsWith(uri, kAppPrefix)) return false;
    const string rest = uri.substr(std::char_traits<char>::length(kAppPrefix));
    const size_t colon = rest.find(':');
    appId = rest.substr(0, colon);
    path = "/";
    if (colon != string::npos)
        for (char c : rest.substr(colon + 1))
            path += (c == ':') ? '/' : c;
    while (path.size() > 1 && path.back() == '/')
        path.pop_back();
    return !appId.empty();
}

// id "<id>", path "/a/b" -> spacify:app:<id>:a:b
string appUri(const string& appId, const string& path) {
    string uri = kAppPrefix + appId;
    for (char c : path)
        uri += (c == '/') ? ':' : c;
    while (uri.back() == ':')
        uri.pop_back();
    return uri;
}

bool hasIndexHtml(const SPAppBundle& bundle) {
    std::error_code ec;
    return std::filesystem::is_regular_file(std::filesystem::u8path(bundle.path) / "index.html", ec);
}

} // namespace
#endif

AppView::AppView(SPElement* parent)
    : SiteView(parent, "spacify:app:", true, getDevMode(), false) // no bookmark button — we add "Add app" instead
    , m_devMode(getDevMode())
{
#ifdef BUNGALOW_CEF
    m_errorView = new SPVBoxElement(this);
    m_errorView->setFlex(1);
    m_errorView->setPadding({ 24, 24, 24, 24 });
    m_errorView->setGap(8);
    auto* heading = new SPTextElement(m_errorView);
    heading->setInnerText("App unavailable");
    heading->setStringAttribute("size", "20");
    heading->setHeight(28);
    m_errorView->appendChild(heading);
    m_errorText = new SPTextElement(m_errorView);
    m_errorText->setHeight(20);
    m_errorView->appendChild(m_errorText);
    m_errorView->setVisible(false);
    this->appendChild(m_errorView);
#endif

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

#ifdef BUNGALOW_CEF
    if (m_devMode) {
        m_devToolsButton = new SPButtonElement(m_navRow);
        m_devToolsButton->setInnerText("DevTools");
        m_devToolsButton->setHeight(kNavBarHeight - 8);
        m_devToolsButton->addEventListener("click",
            reinterpret_cast<SPCallback>(&AppView::onDevToolsClicked), this, m_devToolsButton);
        m_navRow->appendChild(m_devToolsButton);
    }
#endif
}

AppView::AppView(SPElement* parent, bool showNavBar)
    : AppView(parent)
{
    if (!showNavBar) setNavBarVisible(false);
}

bool AppView::acceptsUri(const string& uri) {
    return space::startsWith(uri, "spacify:app:");
}

void AppView::navigate(const string& uri) {
#ifdef BUNGALOW_CEF
    if (navigateLocalApp(uri)) {
        if (!m_devMode)
            updateAppNavBar(uri);
        invalidate(); // the browser is positioned on the next paint
        return;
    }
#endif
    SiteView::navigate(uri);
    if (!m_devMode)
        updateAppNavBar(uri);
}

void AppView::updateAppNavBar(const string& uri) {
    auto* app = SpacifyApp::current();
    if (!app || !app->getDatabase()) return;
    bool favorited = app->getDatabase()->isBookmarked(uri);
    setNavBarVisible(!favorited);
}

int AppView::onAddAppClicked(SPObject* /*sender*/, SPEventArgs* /*args*/) {
    const string& uri = getCurrentUri();
    if (uri.empty()) return 0;
    string title = uri; // fallback; webview title not directly accessible here
#ifdef BUNGALOW_CEF
    if (m_activeApp && !m_activeApp->title.empty())
        title = m_activeApp->title;
#endif
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

#ifdef BUNGALOW_CEF
// ── Local app bundles ─────────────────────────────────────────────────────────

void AppView::navigateBack(const string& uri) {
    if (navigateLocalApp(uri)) {
        if (!m_devMode)
            updateAppNavBar(uri);
        invalidate();
    } else {
        SiteView::navigateBack(uri);
    }
}

void AppView::navigateForward(const string& uri) {
    navigateBack(uri);
}

bool AppView::navigateLocalApp(const string& uri) {
    string appId, path;
    if (!splitAppUri(uri, appId, path)) return false;

    const SPAppBundle* bundle = SPAppManager::instance().findBundle(appId);
    if (bundle && !hasIndexHtml(*bundle)) {
        // Not an HTML5 bundle: keep the remote https://<id>.apps.spacify.se path.
        showLocalApp(nullptr);
        return false;
    }

    setCurrentUri(uri);
    if (!bundle) {
        showError("No app with the id \"" + appId + "\" is installed.");
        return true;
    }
    LocalApp* app = localAppFor(*bundle);
    if (!app) {
        showError("Apps cannot be shown because the embedded browser failed to start.");
        return true;
    }

    showLocalApp(app);
    if (app->route.empty())
        app->browser->loadPath(path);
    else if (app->route != path)
        app->browser->navigateRoute(path);
    app->route = path;
    return true;
}

AppView::LocalApp* AppView::localAppFor(const SPAppBundle& bundle) {
    const string appId = bundle.identifier;
    auto existing = m_localApps.find(appId);
    if (existing != m_localApps.end()) return &existing->second;

    SPEmbeddedBrowser::Options options;
    options.developerMode = m_devMode;
    // Run the theme as soon as the document element exists, before app scripts.
    options.startupScript =
        "(function(apply){if(document.documentElement){apply();return;}"
        "new MutationObserver(function(_,o){if(document.documentElement){o.disconnect();apply();}})"
        ".observe(document,{childList:true});})(function(){" + themeScript() + "});";
    // Besides spacify.se, apps may fetch from the https hosts their manifest
    // lists under AllowedHosts, once the user has accepted each of them.
    options.allowedHosts.insert("spacify.se");
    auto hosts = bundle.manifest.find("AllowedHosts");
    if (hosts != bundle.manifest.end() && hosts->is_array()) {
        for (const auto& entry : *hosts) {
            if (!entry.is_string()) continue;
            const string host = entry.get<string>();
            if (isDomainAllowed("https://" + host) || showThirdPartyWarning(host))
                options.allowedHosts.insert(host);
        }
    }

    SPEmbeddedBrowser::Callbacks callbacks;
    callbacks.routeChanged = [this, appId](const string& path) { onRouteChanged(appId, path); };
    callbacks.navigateRequested = [this](const string& target) { getWindowElement()->navigate(target); };
    callbacks.externalUrlRequested = [this](const string& url) { openExternalUrl(url); };
    callbacks.titleChanged = [this, appId](const string& title) {
        auto app = m_localApps.find(appId);
        if (app != m_localApps.end()) app->second.title = title;
    };

    auto browser = createEmbeddedAppBrowser(bundle, this, std::move(options), std::move(callbacks));
    if (!browser) return nullptr;
    LocalApp& app = m_localApps[appId];
    app.browser = std::move(browser);
    return &app;
}

// The app changed its own route: record it in the view stack history.
void AppView::onRouteChanged(const string& appId, const string& path) {
    auto it = m_localApps.find(appId);
    if (it == m_localApps.end() || it->second.route == path) return;
    it->second.route = path;
    if (&it->second == m_activeApp && isVisible())
        getWindowElement()->navigate(appUri(appId, path));
}

void AppView::showLocalApp(LocalApp* app) {
    if (m_activeApp && m_activeApp != app)
        m_activeApp->browser->setVisible(false);
    m_activeApp = app;
    if (m_errorView) m_errorView->setVisible(false);
    invalidate();
}

void AppView::showError(const string& message) {
    showLocalApp(nullptr);
    m_errorText->setInnerText(message);
    m_errorView->setVisible(true);
    invalidate();
}

void AppView::setVisible(bool value) {
    SiteView::setVisible(value);
    if (!m_activeApp) return;
    if (value) {
        m_activeApp->browser->executeScript(themeScript()); // pick up theme changes
        invalidate();
    } else {
        m_activeApp->browser->setVisible(false); // hidden views are not painted
    }
}

// Absolute bounds are only known once painted, so the native browser is
// positioned and shown from here; layout changes and window resizes both end
// in a repaint.
void AppView::paint(int x, int y, SPGraphicsContext* c) {
    SiteView::paint(x, y, c);
    if (!m_activeApp || !isVisible()) return;
    const auto bounds = getAbsoluteBounds();
    const int navOffset = getNavOffset();
    m_activeApp->browser->setBounds(bounds.x, bounds.y + navOffset, bounds.width, bounds.height - navOffset);
    m_activeApp->browser->setVisible(true);
}

// Exposes the app theme as CSS variables, like the wx SiteView does for sites.
string AppView::themeScript() {
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

int AppView::onDevToolsClicked(SPObject* /*sender*/, SPEventArgs* /*args*/) {
    if (m_activeApp) m_activeApp->browser->showDevTools();
    return 0;
}
#endif

} // namespace views
} // namespace space
