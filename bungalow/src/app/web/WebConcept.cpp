#include "app/web/WebConcept.h"

#include "app/AppContext.h"
#include "concept/ConceptContext.h"
#include "feature/Feature.h"
#include "web/BookmarkEvents.h"
#include "web/BookmarkManager.h"

#include "AppView.h"
#include "CuratorView.h"
#include "Database.h"
#include "SiteView.h"
#include "StoreView.h"

namespace space {
namespace app {
namespace {

/// The APPS and SITES sections: one item per bookmarked app or site, with
/// its bookmarked sub-pages as children.
class BookmarksFeature : public feature::Feature {
public:
    explicit BookmarksFeature(const FaviconCache& favicons) : m_favicons(favicons) {}

    std::string getId() const override { return "web.bookmarks"; }
    std::string getName() const override { return "Bookmarks"; }

    std::vector<navigation::SidebarNode> getSidebarNodes() const override {
        return {section("APPS", "bungalow:app:", "icons/App.svg"),
                section("SITES", "bungalow:site:", "icons/WebPage.svg")};
    }

private:
    navigation::SidebarNode section(const std::string& label, const std::string& prefix,
                                    const std::string& defaultIcon) const {
        navigation::SidebarNode result(label);
        Database* db = database();
        if (!db) return result;
        for (const auto& root : web::BookmarkManager::roots(*db, prefix)) {
            navigation::SidebarNode rootNode = node(root, defaultIcon);
            rootNode.setExpanded(true);
            for (const auto& child : web::BookmarkManager::childrenOf(*db, root))
                rootNode.addChild(node(child, defaultIcon));
            result.addChild(std::move(rootNode));
        }
        return result;
    }

    navigation::SidebarNode node(const Database::Bookmark& bookmark, const std::string& defaultIcon) const {
        navigation::SidebarNode result(web::BookmarkManager::displayTitle(bookmark), bookmark.uri);
        result.setBookmark(true).setIconPath(iconFor(bookmark.uri, defaultIcon));
        return result;
    }

    // The cached favicon if there is one, otherwise the default icon.
    std::string iconFor(const std::string& uri, const std::string& fallback) const {
        const std::string domain = web::BookmarkManager::faviconDomain(uri);
        if (domain.empty()) return fallback;
        const std::string cached = m_favicons.getCachedPath(domain);
        return cached.empty() ? fallback : cached;
    }

    const FaviconCache& m_favicons;
};

} // namespace

WebConcept::WebConcept() : m_favicons(std::make_unique<FaviconCache>()) {}

WebConcept::~WebConcept() = default;

void WebConcept::onActivate(concepts::ConceptContext& ctx) {
    m_ctx = &ctx;
    SPViewStackElement* viewStack = ctx.viewStack();
    ctx.registerView(new views::SiteView(viewStack));
    ctx.registerView(new views::AppView(viewStack));
    ctx.registerView(new views::CuratorView(viewStack));
    ctx.registerView(new views::StoreView(viewStack));
    ctx.registerFeature(std::make_unique<BookmarksFeature>(*m_favicons));

    m_bookmarkListener = web::BookmarkEvents::addListener([this](const web::BookmarkChange& change) {
        if (!m_ctx) return;
        m_ctx->refreshSidebar();
        if (!change.added) return;
        // Fetch the favicon of a newly bookmarked site or app.
        const std::string domain = web::BookmarkManager::faviconDomain(change.uri);
        if (domain.empty()) return;
        m_favicons->fetchIfNeeded(domain, "https://" + domain + "/favicon.ico", [this] {
            if (m_ctx) m_ctx->refreshSidebar();
        });
    });
}

void WebConcept::onDeactivate() {
    web::BookmarkEvents::removeListener(m_bookmarkListener);
    m_bookmarkListener = 0;
    m_ctx = nullptr;
}

} // namespace app
} // namespace space
