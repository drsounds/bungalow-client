#pragma once

#include "datasource/Node.h"
#include "models/SPArtist.h"
#include "models/SPRecording.h"
#include "models/SPRelease.h"
#include "models/SPTrack.h"

#include <optional>
#include <string>
#include <vector>

namespace space {

/** The columns a music table can show. Each entity answers the ones it knows. */
enum class MusicColumn {
    Name,
    Version,
    Artists,
    Release,        // the release's name, "" without one
    Appearance,     // the release's name, "(no release)" without one
    Date,
    Number,         // position on the release (or in a playlist)
    Saved,
    TrackCount,
    RecordingCount,
    ReleaseCount,
    Isni,
    Isrc,
    Upc,
};

/** The heading of @p column. */
std::string musicColumnTitle(MusicColumn column);

/**
 * MusicEntity — a row of a music table, backed by the ORM models it was
 * loaded from. Subclasses are the library's artists, recordings, releases
 * and tracks (and a playlist's items).
 */
class MusicEntity : public MediaEntity {
public:
    /** The cell text for @p column, "" for columns this entity doesn't have. */
    virtual std::string text(MusicColumn column) const;
    /**
     * The cell as names that can be clicked: every artist of the Artists
     * column, the release of the Release columns. Empty for a column that
     * is plain text; otherwise the names, joined by ", ", are text().
     */
    virtual std::vector<EntityLink> links(MusicColumn column) const;
};

/** An artist, with how much of the library credits them. */
class ArtistEntity final : public MusicEntity {
public:
    ArtistEntity(SPArtist artist, int recordingCount, int releaseCount);

    const SPArtist& artist() const { return m_artist; }

    std::string id() const override { return m_artist.isni; }
    std::string name() const override { return m_artist.name; }
    std::string uri() const override { return "musik:isni:" + m_artist.isni; }
    bool        isPlayable() const override { return true; } // not greyed out: rows open the artist page
    std::string text(MusicColumn column) const override;

private:
    SPArtist m_artist;
    int      m_recordingCount = 0;
    int      m_releaseCount   = 0;
};

/** A recording with its credited artists. It plays through one of its tracks. */
class RecordingEntity final : public MusicEntity {
public:
    /** @p trackId and @p playUri are the track the recording plays through, 0 / "" when it has none. */
    RecordingEntity(SPRecording recording, std::vector<SPArtist> artists,
                    int64_t trackId, std::string playUri);

    const SPRecording&           recording() const { return m_recording; }
    const std::vector<SPArtist>& artists() const { return m_artists; }

    std::string id() const override { return m_recording.isrc; }
    std::string name() const override { return m_recording.name; }
    std::string uri() const override { return "musik:isrc:" + m_recording.isrc; }
    std::string playUri() const override { return m_playUri; }
    std::string artistNames() const override;
    std::vector<EntityLink> artistLinks() const override;
    int64_t     trackId() const override { return m_trackId; }
    std::string text(MusicColumn column) const override;

private:
    SPRecording           m_recording;
    std::vector<SPArtist> m_artists;
    int64_t               m_trackId = 0;
    std::string           m_playUri;
};

/** A release with its credited artists and track count. */
class ReleaseEntity final : public MusicEntity {
public:
    ReleaseEntity(SPRelease release, std::vector<SPArtist> artists, int trackCount);

    const SPRelease&             release() const { return m_release; }
    const std::vector<SPArtist>& artists() const { return m_artists; }

    std::string id() const override { return m_release.upc; }
    std::string name() const override { return m_release.name; }
    std::string uri() const override { return "musik:upc:" + m_release.upc; }
    bool        isPlayable() const override { return true; } // not greyed out: rows open the release page
    std::string artistNames() const override;
    std::vector<EntityLink> artistLinks() const override;
    std::string text(MusicColumn column) const override;

private:
    SPRelease             m_release;
    std::vector<SPArtist> m_artists;
    int                   m_trackCount = 0;
};

/**
 * A track: a recording's appearance on a release (or on none). The track
 * model's recording / release relations point at the copies held here.
 */
class TrackEntity final : public MusicEntity {
public:
    /** @p artists are the recording's credits; @p playUri is "" when the track has no playable. */
    TrackEntity(SPTrack track, SPRecording recording, std::optional<SPRelease> release,
                std::vector<SPArtist> artists, std::string playUri, bool saved);

    const SPTrack&               track() const { return m_track; }
    const SPRecording&           recording() const { return m_recording; }
    /** nullptr for a track without a release. */
    const SPRelease*             release() const { return m_release ? &*m_release : nullptr; }
    const std::vector<SPArtist>& artists() const { return m_artists; }
    bool                         saved() const { return m_saved; }

    std::string id() const override { return std::to_string(m_track.id); }
    std::string name() const override { return m_recording.name; }
    std::string uri() const override { return "musik:isrc:" + m_recording.isrc; }
    std::string playUri() const override { return m_playUri; }
    std::string artistNames() const override;
    std::vector<EntityLink> artistLinks() const override;
    int64_t     trackId() const override { return m_track.id; }
    std::string isrc() const override { return m_recording.isrc; }
    std::string text(MusicColumn column) const override;
    std::vector<EntityLink> links(MusicColumn column) const override;

private:
    SPTrack                  m_track;
    SPRecording              m_recording;
    std::optional<SPRelease> m_release;
    std::vector<SPArtist>    m_artists;
    std::string              m_playUri;
    bool                     m_saved = false;
};

namespace views {

/** An identifier for display: generated "local:" keys show as "". */
std::string displayId(const std::string& key);

} // namespace views
} // namespace space
