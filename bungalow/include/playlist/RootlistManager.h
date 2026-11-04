#pragma once

#include "Database.h"

#include <string>
#include <vector>

namespace space {
namespace playlist {

/// The user's rootlist (bungalow:rootlist): the ordered list of playlists
/// shown at the end of the sidebar. Playlists are federated (RFC-0001): each
/// is listed by its musik:federated:user:<handle>:playlist:<uuid> link. Every
/// change fires RootlistEvents.
class RootlistManager {
public:
    RootlistManager() = delete;

    static constexpr const char* kRootlistUri = "bungalow:rootlist";

    /// The rootlist, top first.
    static std::vector<Database::RootlistNode> nodes(Database& db);

    /// Creates a playlist named name, owned by handle, at the top of the
    /// rootlist. Returns its link, or "" if it could not be created.
    static std::string createPlaylist(Database& db, const std::string& name, const std::string& handle);
    /// Renames a node, and the playlist if it is one.
    static void rename(Database& db, const std::string& uri, const std::string& title);
    /// Moves a node to position (0 = top).
    static void move(Database& db, const std::string& uri, int position);
    /// Removes a node; a playlist is deleted with it.
    static void remove(Database& db, const std::string& uri);

    /// Relists playlists created before they were federated
    /// (bungalow:playlist:<uuid>) under their federated link, owned by handle.
    static void migrateLegacyUris(Database& db, const std::string& handle);

    /// The id of the playlist a federated link points to, or "" for any other URI.
    static std::string playlistId(const std::string& uri);
};

} // namespace playlist
} // namespace space
