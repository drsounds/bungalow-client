#include "playlist/RootlistManager.h"

#include "playlist/RootlistEvents.h"
#include "playlist/Upsl.h"

namespace space {
namespace playlist {

std::vector<Database::RootlistNode> RootlistManager::nodes(Database& db) {
    return db.getRootlist();
}

std::string RootlistManager::createPlaylist(Database& db, const std::string& name, const std::string& handle) {
    const std::string id = db.createPlaylist(name, handle);
    if (id.empty()) return {};
    const std::string uri = Upsl::hostedUri(handle, id);
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

void RootlistManager::migrateLegacyUris(Database& db, const std::string& handle) {
    const std::string legacyPrefix = "bungalow:playlist:";
    bool changed = false;
    for (const auto& node : db.getRootlist()) {
        if (node.uri.rfind(legacyPrefix, 0) != 0) continue;
        const std::string id = node.uri.substr(legacyPrefix.size());
        if (id.empty() || id.find(':') != std::string::npos) continue;
        db.setRootlistNodeUri(node.uri, Upsl::hostedUri(handle, id));
        db.setPlaylistHandle(id, handle);
        changed = true;
    }
    if (changed) RootlistEvents::fireChanged();
}

std::string RootlistManager::playlistId(const std::string& uri) {
    const auto upsl = Upsl::parse(uri);
    return upsl ? upsl->playlistId : std::string();
}

} // namespace playlist
} // namespace space
