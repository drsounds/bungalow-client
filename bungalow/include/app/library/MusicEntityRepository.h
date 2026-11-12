#pragma once

#include "app/library/MusicEntity.h"
#include "models/SPRecordingArtist.h"
#include "models/SPReleaseArtist.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Database;

namespace space {

/**
 * MusicEntityRepository
 *
 * A snapshot of the music library, loaded through the ORM models and joined
 * in memory. The entities it hands out own copies of their models, so they
 * outlive the repository; views build one per reload.
 *
 * If the database cannot be read the snapshot is empty.
 */
class MusicEntityRepository {
public:
    using Artists    = std::vector<std::shared_ptr<const ArtistEntity>>;
    using Recordings = std::vector<std::shared_ptr<const RecordingEntity>>;
    using Releases   = std::vector<std::shared_ptr<const ReleaseEntity>>;
    using Tracks     = std::vector<std::shared_ptr<const TrackEntity>>;

    explicit MusicEntityRepository(Database& db);

    MusicEntityRepository(const MusicEntityRepository&)            = delete;
    MusicEntityRepository& operator=(const MusicEntityRepository&) = delete;

    /** Single lookups; nullptr when there is no such entity. */
    std::shared_ptr<const ArtistEntity>    artist(const std::string& isni) const;
    std::shared_ptr<const RecordingEntity> recording(const std::string& isrc) const;
    std::shared_ptr<const ReleaseEntity>   release(const std::string& upc) const;

    /** Browse lists, ordered by name (case-insensitive). */
    Artists    artists() const;
    Recordings recordings() const;
    Releases   releases() const;

    /** Every track, ordered by release then position; tracks without a release last. */
    Tracks tracks() const;
    /** Tracks on one release: numbered ones in order, then the rest as inserted. */
    Tracks tracksForRelease(const std::string& upc) const;
    /** Tracks of every recording the artist is credited on. */
    Tracks tracksForArtist(const std::string& isni) const;
    /** Tracks of one recording (one per release it appears on), oldest release first. */
    Tracks tracksForRecording(const std::string& isrc) const;
    /** Releases the artist is credited on, or that contain their recordings. */
    Releases releasesForArtist(const std::string& isni) const;

private:
    std::vector<SPArtist> creditsOf(const std::string& key, bool release) const;
    std::shared_ptr<const ReleaseEntity> makeRelease(const SPRelease& release) const;
    /** Entities for @p tracks, in that order; tracks whose recording is missing are skipped. */
    Tracks makeTracks(std::vector<const SPTrack*> tracks) const;
    const SPRelease* releaseOf(const SPTrack& track) const;

    std::vector<SPArtist>          m_artists;
    std::vector<SPRecording>       m_recordings;
    std::vector<SPRelease>         m_releases;
    std::vector<SPTrack>           m_tracks;
    std::vector<SPRecordingArtist> m_recordingArtists; // in credit order
    std::vector<SPReleaseArtist>   m_releaseArtists;   // in credit order

    // Indexes into the vectors above, which don't change after loading.
    std::unordered_map<std::string, const SPArtist*>    m_artistByIsni;
    std::unordered_map<std::string, const SPRecording*> m_recordingByIsrc;
    std::unordered_map<std::string, const SPRelease*>   m_releaseByUpc;

    std::unordered_map<int64_t, std::string> m_playUriByTrack;
    std::unordered_set<int64_t>              m_savedTracks;
};

} // namespace space
