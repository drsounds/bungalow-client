#pragma once

#include "service/Service.h"

#include <optional>
#include <string>
#include <vector>

namespace space {
namespace service {

/// Discovery capability: search and browse a music catalogue (MusicBrainz,
/// Discogs, …). It needn't play anything; a back-end that can also stream
/// implements MusicService as well. Entities carry the catalogue's own id plus
/// the portable identifiers (ISRC, UPC, ISNI) when the catalogue knows them,
/// so a hit can be resolved for playback or saved to the library.
class MusicCatalogueService : public virtual Service {
public:
    struct Artist {
        std::string id;    // catalogue id, e.g. a MusicBrainz MBID
        std::string isni;
        std::string name;
    };

    struct Release {
        std::string id;
        std::string upc;
        std::string title;
        std::string artist;      // credited artist names, display form
        std::string releaseDate; // ISO 8601, possibly partial ("1977")
        std::string coverArtUri;
    };

    struct Recording {
        std::string id;
        std::string isrc;
        std::string title;
        std::string artist;
        int         durationMs = 0;
    };

    virtual std::vector<Artist>    searchArtists(const std::string& query) = 0;
    virtual std::vector<Release>   searchReleases(const std::string& query) = 0;
    virtual std::vector<Recording> searchRecordings(const std::string& query) = 0;

    virtual std::optional<Artist>    getArtistByIsni(const std::string& isni) = 0;
    virtual std::optional<Release>   getReleaseByUpc(const std::string& upc) = 0;
    virtual std::optional<Recording> getRecordingByIsrc(const std::string& isrc) = 0;

    // Most catalogues have no global "list everything" feed, so browsing hangs
    // off a parent entity (artist → releases → recordings). Catalogues that
    // can't browse return nothing.

    /// Releases credited to the artist with catalogue id @p artistId.
    virtual std::vector<Release> browseReleasesByArtist(const std::string& /*artistId*/,
                                                        int /*offset*/, int /*limit*/) {
        return {};
    }

    /// Recordings on the release with catalogue id @p releaseId.
    virtual std::vector<Recording> browseRecordingsByRelease(const std::string& /*releaseId*/,
                                                             int /*offset*/, int /*limit*/) {
        return {};
    }
};

} // namespace service
} // namespace space
