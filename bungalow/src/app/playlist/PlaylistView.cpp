#include "app/playlist/PlaylistView.h"

#include "app/AppContext.h"
#include "playlist/RootlistManager.h"

#include "Database.h"
#include "SPHeaderElement.h"
#include "SPTitleElement.h"

namespace space {
namespace app {

PlaylistView::PlaylistView(SPElement* parent) : SPViewElement(parent) {
    this->setPadding({ 20, 20, 20, 20 });
    m_header = new SPHeaderElement(this);
    m_header->title->setEntity("Playlist");
    m_header->set("width", "100%");
    m_header->set("height", "180");
    this->appendChild(m_header);
}

bool PlaylistView::acceptsUri(const std::string& uri) {
    return !playlist::RootlistManager::playlistId(uri).empty();
}

void PlaylistView::navigate(const std::string& uri) {
    m_uri = uri;
    refresh();
}

void PlaylistView::refresh() {
    Database* db = database();
    const std::string id = playlist::RootlistManager::playlistId(m_uri);
    if (!db || id.empty()) return;
    const std::string name = db->getPlaylistName(id);
    m_header->title->setTitle(name.empty() ? "Playlist not found" : name);
    this->invalidate();
}

} // namespace app
} // namespace space
