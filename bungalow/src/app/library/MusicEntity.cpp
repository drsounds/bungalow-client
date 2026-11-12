#include "app/library/MusicEntity.h"

#include <utility>

namespace space {
namespace {

// "A, B, C" in credit order.
std::string joinNames(const std::vector<SPArtist>& artists) {
    std::string names;
    for (const auto& artist : artists) {
        if (!names.empty()) names += ", ";
        names += artist.name;
    }
    return names;
}

// The library's detail pages; see views::libraryUri().
const char* const kArtistPage  = "bungalow:library:artist:";
const char* const kReleasePage = "bungalow:library:release:";

// One link per artist, to the artist's page, in credit order.
std::vector<EntityLink> artistPageLinks(const std::vector<SPArtist>& artists) {
    std::vector<EntityLink> links;
    links.reserve(artists.size());
    for (const auto& artist : artists)
        links.push_back({artist.name, artist.isni.empty() ? std::string() : kArtistPage + artist.isni});
    return links;
}

} // namespace

namespace views {

std::string displayId(const std::string& key) {
    return key.rfind("local:", 0) == 0 ? "" : key;
}

} // namespace views

std::string musicColumnTitle(MusicColumn column) {
    switch (column) {
    case MusicColumn::Name:           return "Name";
    case MusicColumn::Version:        return "Version";
    case MusicColumn::Artists:        return "Artists";
    case MusicColumn::Release:        return "Release";
    case MusicColumn::Appearance:     return "Release";
    case MusicColumn::Date:           return "Date";
    case MusicColumn::Number:         return "#";
    case MusicColumn::Saved:          return "Saved";
    case MusicColumn::TrackCount:     return "Tracks";
    case MusicColumn::RecordingCount: return "Recordings";
    case MusicColumn::ReleaseCount:   return "Releases";
    case MusicColumn::Isni:           return "ISNI";
    case MusicColumn::Isrc:           return "ISRC";
    case MusicColumn::Upc:            return "UPC";
    }
    return "";
}

// ── MusicEntity ───────────────────────────────────────────────────────────────

std::string MusicEntity::text(MusicColumn column) const {
    switch (column) {
    case MusicColumn::Name:    return name();
    case MusicColumn::Artists: return artistNames();
    default:                   return "";
    }
}

std::vector<EntityLink> MusicEntity::links(MusicColumn column) const {
    return column == MusicColumn::Artists ? artistLinks() : std::vector<EntityLink>();
}

// ── ArtistEntity ──────────────────────────────────────────────────────────────

ArtistEntity::ArtistEntity(SPArtist artist, int recordingCount, int releaseCount)
    : m_artist(std::move(artist)), m_recordingCount(recordingCount), m_releaseCount(releaseCount) {}

std::string ArtistEntity::text(MusicColumn column) const {
    switch (column) {
    case MusicColumn::Isni:           return views::displayId(m_artist.isni);
    case MusicColumn::RecordingCount: return std::to_string(m_recordingCount);
    case MusicColumn::ReleaseCount:   return std::to_string(m_releaseCount);
    default:                          return MusicEntity::text(column);
    }
}

// ── RecordingEntity ───────────────────────────────────────────────────────────

RecordingEntity::RecordingEntity(SPRecording recording, std::vector<SPArtist> artists,
                                 int64_t trackId, std::string playUri)
    : m_recording(std::move(recording)), m_artists(std::move(artists)),
      m_trackId(trackId), m_playUri(std::move(playUri)) {}

std::string RecordingEntity::artistNames() const {
    return joinNames(m_artists);
}

std::vector<EntityLink> RecordingEntity::artistLinks() const {
    return artistPageLinks(m_artists);
}

std::string RecordingEntity::text(MusicColumn column) const {
    switch (column) {
    case MusicColumn::Version: return m_recording.version;
    case MusicColumn::Isrc:    return views::displayId(m_recording.isrc);
    default:                   return MusicEntity::text(column);
    }
}

// ── ReleaseEntity ─────────────────────────────────────────────────────────────

ReleaseEntity::ReleaseEntity(SPRelease release, std::vector<SPArtist> artists, int trackCount)
    : m_release(std::move(release)), m_artists(std::move(artists)), m_trackCount(trackCount) {}

std::string ReleaseEntity::artistNames() const {
    return joinNames(m_artists);
}

std::vector<EntityLink> ReleaseEntity::artistLinks() const {
    return artistPageLinks(m_artists);
}

std::string ReleaseEntity::text(MusicColumn column) const {
    switch (column) {
    case MusicColumn::Date:       return m_release.releaseDate;
    case MusicColumn::TrackCount: return std::to_string(m_trackCount);
    case MusicColumn::Upc:        return views::displayId(m_release.upc);
    default:                      return MusicEntity::text(column);
    }
}

// ── TrackEntity ───────────────────────────────────────────────────────────────

TrackEntity::TrackEntity(SPTrack track, SPRecording recording, std::optional<SPRelease> release,
                         std::vector<SPArtist> artists, std::string playUri, bool saved)
    : m_track(std::move(track)), m_recording(std::move(recording)), m_release(std::move(release)),
      m_artists(std::move(artists)), m_playUri(std::move(playUri)), m_saved(saved) {
    m_track.recording = &m_recording;
    m_track.release   = m_release ? &*m_release : nullptr;
}

std::string TrackEntity::artistNames() const {
    return joinNames(m_artists);
}

std::vector<EntityLink> TrackEntity::artistLinks() const {
    return artistPageLinks(m_artists);
}

std::vector<EntityLink> TrackEntity::links(MusicColumn column) const {
    const bool releaseColumn = column == MusicColumn::Release || column == MusicColumn::Appearance;
    if (releaseColumn && m_release && !m_release->name.empty() && !m_release->upc.empty())
        return {{m_release->name, kReleasePage + m_release->upc}};
    return MusicEntity::links(column);
}

std::string TrackEntity::text(MusicColumn column) const {
    switch (column) {
    case MusicColumn::Version:    return m_recording.version;
    case MusicColumn::Release:    return m_release ? m_release->name : "";
    case MusicColumn::Appearance: return m_release && !m_release->name.empty() ? m_release->name : "(no release)";
    case MusicColumn::Date:       return m_release ? m_release->releaseDate : "";
    case MusicColumn::Number:     return m_track.number > 0 ? std::to_string(m_track.number) : "";
    case MusicColumn::Saved:      return m_saved ? "Yes" : "";
    case MusicColumn::Isrc:       return views::displayId(m_recording.isrc);
    case MusicColumn::Upc:        return m_release ? views::displayId(m_release->upc) : "";
    default:                      return MusicEntity::text(column);
    }
}

} // namespace space
