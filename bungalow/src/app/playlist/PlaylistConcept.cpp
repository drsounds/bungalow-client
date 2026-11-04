#include "app/playlist/PlaylistConcept.h"

#include "app/AppContext.h"
#include "app/playlist/PlaylistView.h"
#include "concept/ConceptContext.h"
#include "feature/Feature.h"
#include "playlist/RootlistEvents.h"
#include "playlist/RootlistManager.h"

#include "ConfigView.h"
#include "Database.h"
#include "PlatformUi.h"

namespace space {
namespace app {
namespace {

using playlist::RootlistManager;

/// The rootlist, after a gap at the end of the sidebar: "New playlist"
/// first, then the playlists in the user's order.
class RootlistFeature : public feature::Feature {
public:
    std::string getId() const override { return "playlist.rootlist"; }
    std::string getName() const override { return "Rootlist"; }

    std::vector<navigation::SidebarNode> getSidebarNodes() const override {
        navigation::SidebarNode section("ROOTLIST");
        section.setSpacer(true);

        navigation::SidebarNode newPlaylist("New playlist", RootlistManager::kRootlistUri);
        newPlaylist.setIconPath("icons/Document.svg")
            .setEditOnClick(true)
            .setOnRename([](const std::string& name) {
                if (Database* db = database())
                    RootlistManager::createPlaylist(*db, name, views::ConfigView::getFederatedHandle());
            });
        section.addChild(std::move(newPlaylist));

        Database* db = database();
        if (!db) return {section};
        for (const auto& node : RootlistManager::nodes(*db)) {
            const std::string uri = node.uri;
            const std::string title = node.title;
            navigation::SidebarNode item(title, uri);
            item.setIconPath("icons/MusicNote.svg")
                .setOnRename([uri](const std::string& name) {
                    if (Database* db = database()) RootlistManager::rename(*db, uri, name);
                })
                .setOnMove([uri](int index) {
                    if (Database* db = database()) RootlistManager::move(*db, uri, index);
                })
                .setOnRemove([uri, title] {
                    if (!platform::showConfirmDialog("Delete playlist", "Delete \"" + title + "\"?"))
                        return;
                    if (Database* db = database()) RootlistManager::remove(*db, uri);
                });
            section.addChild(std::move(item));
        }
        return {section};
    }
};

} // namespace

void PlaylistConcept::onActivate(concepts::ConceptContext& ctx) {
    m_ctx = &ctx;
    // Playlists made before they were federated become the user's.
    if (Database* db = database())
        playlist::RootlistManager::migrateLegacyUris(*db, views::ConfigView::getFederatedHandle());

    m_view = new PlaylistView(ctx.viewStack(), database());
    ctx.registerView(m_view);
    ctx.registerFeature(std::make_unique<RootlistFeature>());

    m_rootlistListener = playlist::RootlistEvents::addListener([this] {
        if (!m_ctx) return;
        m_ctx->refreshSidebar();
        if (m_view) m_view->refresh();
    });
}

void PlaylistConcept::onDeactivate() {
    playlist::RootlistEvents::removeListener(m_rootlistListener);
    m_rootlistListener = 0;
    m_view = nullptr;
    m_ctx = nullptr;
}

} // namespace app
} // namespace space
