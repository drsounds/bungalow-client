#pragma once

#include "service/MediaService.h"

#include <optional>
#include <string>
#include <vector>

namespace space {
namespace service {

/// Music-resolution capability on top of MediaService: can this back-end play
/// a track identified by ISRC, or by title and artist? This is what lets one
/// library row play from local files today and a streaming service tomorrow —
/// the PlaybackCoordinator asks every MusicService at play time.
class MusicService : public MediaService {
public:
    /// One concrete thing this service can play for a request. A non-empty
    /// uri plays exactly that item through loadUri(); otherwise byIsrc picks
    /// between loadByIsrc() and loadByTitleArtist().
    struct Candidate {
        std::string uri;
        std::string title;
        std::string artist;
        bool        byIsrc = false;
    };

    /// Non-loading lookup by ISRC. Should be cheap: it also runs when a track
    /// is merely about to play.
    virtual std::optional<Candidate> lookup(const std::string& isrc) = 0;

    /// Non-loading, best-effort lookup by title and artist (artist may be
    /// empty). Finds nothing by default.
    virtual std::optional<Candidate> lookupByTitleArtist(const std::string& title,
                                                         const std::string& artist);

    /// Every candidate to offer in the "Play with…" chooser. The default is
    /// the ISRC lookup, else the title/artist one; a service that can offer
    /// several distinct items (search results) overrides it and sets a uri on
    /// each, so the pick replays that exact item.
    virtual std::vector<Candidate> findCandidates(const std::string& isrc, const std::string& title,
                                                  const std::string& artist);

    /// Load the recording with this ISRC. The default loads the lookup()'s uri.
    virtual void loadByIsrc(const std::string& isrc);
    /// Load by title and artist. The default loads the lookupByTitleArtist()'s uri.
    virtual void loadByTitleArtist(const std::string& title, const std::string& artist);
};

} // namespace service
} // namespace space
