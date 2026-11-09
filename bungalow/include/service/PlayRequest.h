#pragma once

#include <cstdint>
#include <string>

namespace space {
namespace service {

/// A vendor-neutral request to play one track. A view doesn't say "play this
/// file" or "play this stream": it hands over whatever identity it has, and
/// the PlaybackCoordinator resolves it across the installed MusicServices.
struct PlayRequest {
    int64_t     trackId = 0;  // library track (tracks.id), 0 when not from the library
    std::string isrc;
    std::string title;
    std::string artist;
    /// A concrete item for the track, such as the row's own file. The Service
    /// that accepts it offers exactly this item; when no Service resolves the
    /// track it is played directly.
    std::string fallbackUri;
    int         durationMs = 0;

    /// Stable identity under which a "Play with…" pick is remembered when the
    /// request has no library track: the ISRC, else the fallback URI, else
    /// title and artist.
    std::string trackKey() const {
        if (!isrc.empty()) return "isrc:" + isrc;
        if (!fallbackUri.empty()) return fallbackUri;
        return "ta:" + title + "|" + artist;
    }
};

} // namespace service
} // namespace space
