#pragma once

#include <optional>
#include <string>
#include <vector>

namespace space {
namespace playlist {

/// Base62 for UPSL leaves (RFC-0001 §4.1): the bytes as a big-endian number in
/// 0-9A-Za-z, with each leading zero byte kept as a leading '0'.
namespace base62 {
std::string encode(const std::string& bytes);
/// nullopt if a character is outside the alphabet.
std::optional<std::string> decode(const std::string& text);
} // namespace base62

/// A Universal Playlist Sharing Link (RFC-0001):
///
///   musik:federated:user:<handle>:playlist:<uuid>[:<body>][:sig:<signature>]
///
/// A link without a body is a hosted reference: the playlist with that id on
/// the handle's instance (for the user's own playlists, the local database).
/// With a body the link carries the playlist itself: its name, description
/// and content references. The legacy bungalow:federated: and
/// spacify:federated: spellings are read as well.
struct Upsl {
    static constexpr const char* kPrefix = "musik:federated:user:";

    std::string handle;      ///< @user@instance
    std::string playlistId;  ///< UUIDv4
    bool embedded = false;   ///< has a body
    std::string name;
    std::string description;
    std::vector<std::string> refs;  ///< content URIs, normally musik:

    /// nullopt if uri is not a UPSL or a leaf does not decode (RFC-0001 §12).
    static std::optional<Upsl> parse(const std::string& uri);
    static bool isUpsl(const std::string& uri);
    /// uri with a legacy federated prefix replaced by musik:federated:user:.
    static std::string canonicalUri(const std::string& uri);

    /// musik:federated:user:<handle>:playlist:<id>
    static std::string hostedUri(const std::string& handle, const std::string& playlistId);
    /// The link with the body embedded.
    std::string embeddedUri() const;

    /// "@localpart@domain" (RFC-0001 §5).
    static bool isValidHandle(const std::string& handle);
    /// @<user name>@localhost, until the user sets their own handle.
    static std::string defaultHandle();
};

/// Percent-decodes a name segment of a musik: content ref (RFC-0002 §4.1).
std::string percentDecode(const std::string& text);

} // namespace playlist
} // namespace space
