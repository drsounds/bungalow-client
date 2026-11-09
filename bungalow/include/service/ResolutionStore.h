#pragma once

#include "service/PlayRequest.h"
#include "service/ServiceMatch.h"

#include <sqlite3.h>
#include <optional>
#include <string>

namespace space {
namespace service {

/// The user's remembered "Play with…" picks, in the music_service_tracks
/// table. A pick is bound to the library track when the play came from the
/// library, and always to the request's PlayRequest::trackKey(), so it is
/// found from any play source. It records enough of the match to replay it
/// without searching again.
class ResolutionStore {
public:
    struct Pick {
        std::string serviceId;
        std::string matchIsrc;   // set when the service matched by ISRC
        std::string matchTitle;
        std::string matchArtist;
        std::string matchUri;    // the exact item, when the service offered one
    };

    /// @p db must outlive the store and contain the music_service_tracks
    /// table, which Database creates.
    explicit ResolutionStore(sqlite3* db) : m_db(db) {}

    /// The remembered pick for this request, preferring one bound to its
    /// library track; nullopt if there is none.
    std::optional<Pick> find(const PlayRequest& request) const;

    /// Remember @p match for this request, replacing any earlier pick.
    bool save(const PlayRequest& request, const ServiceMatch& match);

    /// Forget the pick for this request.
    void clear(const PlayRequest& request);

private:
    sqlite3* m_db;
};

} // namespace service
} // namespace space
