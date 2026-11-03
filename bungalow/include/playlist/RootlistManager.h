#pragma once

#include "Database.h"

#include <string>
#include <vector>

namespace space {
namespace playlist {

/// The user's rootlist (bungalow:rootlist): the ordered list of playlists
/// shown at the end of the sidebar. Every change fires RootlistEvents.
class RootlistManager {
public:
    RootlistManager() = delete;

    static constexpr const char* kRootlistUri = "bungalow:rootlist";
    static constexpr const char* kPlaylistPrefix = "bungalow:playlist:";

    /// The rootlist, top first.
    static std::vector<Database::RootlistNode> nodes(Database& db);

    /// Creates a playlist named name at the top of the rootlist. Returns its
    /// URI, or "" if it could not be created.
    static std::string createPlaylist(Database& db, const std::string& name);
    /// Renames a node, and the playlist if it is one.
    static void rename(Database& db, const std::string& uri, const std::string& title);
    /// Moves a node to position (0 = top).
    static void move(Database& db, const std::string& uri, int position);
    /// Removes a node; a playlist is deleted with it.
    static void remove(Database& db, const std::string& uri);

    /// bungalow:playlist:<id> -> <id>, or "" for any other URI.
    static std::string playlistId(const std::string& uri);
};

} // namespace playlist
} // namespace space
