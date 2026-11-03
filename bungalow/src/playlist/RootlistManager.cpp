#include "playlist/RootlistManager.h"

#include "playlist/RootlistEvents.h"

namespace space {
namespace playlist {

std::vector<Database::RootlistNode> RootlistManager::nodes(Database& db) {
    return db.getRootlist();
}

std::string RootlistManager::createPlaylist(Database& db, const std::string& name) {
    const std::string id = db.createPlaylist(name);
    if (id.empty()) return {};
    const std::string uri = kPlaylistPrefix + id;
    if (!db.insertRootlistNode(uri, name, 0)) {
        db.deletePlaylist(id);
        return {};
    }
    RootlistEvents::fireChanged();
    return uri;
}

void RootlistManager::rename(Database& db, const std::string& uri, const std::string& title) {
    db.renameRootlistNode(uri, title);
    const std::string id = playlistId(uri);
    if (!id.empty()) db.renamePlaylist(id, title);
    RootlistEvents::fireChanged();
}

void RootlistManager::move(Database& db, const std::string& uri, int position) {
    db.moveRootlistNode(uri, position);
    RootlistEvents::fireChanged();
}

void RootlistManager::remove(Database& db, const std::string& uri) {
    db.removeRootlistNode(uri);
    const std::string id = playlistId(uri);
    if (!id.empty()) db.deletePlaylist(id);
    RootlistEvents::fireChanged();
}

std::string RootlistManager::playlistId(const std::string& uri) {
    const std::string prefix = kPlaylistPrefix;
    if (uri.rfind(prefix, 0) != 0) return {};
    const std::string id = uri.substr(prefix.size());
    return id.find(':') == std::string::npos ? id : std::string();
}

} // namespace playlist
} // namespace space
