#include "app/library/MusicEntityRepository.h"

#include "Database.h"
#include "models/SPDBConnection.h"
#include "models/SPPlayable.h"
#include "models/SPPlayableTrack.h"
#include "models/SPSave.h"

#include <algorithm>
#include <exception>

namespace space {
namespace {

// Three-way comparison ignoring ASCII case, as SQLite's NOCASE collation does.
int compareNoCase(const std::string& a, const std::string& b) {
    const auto lower = [](unsigned char c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; };
    const std::size_t n = std::min(a.size(), b.size());
    for (std::size_t i = 0; i < n; ++i) {
        const int ca = lower(static_cast<unsigned char>(a[i]));
        const int cb = lower(static_cast<unsigned char>(b[i]));
        if (ca != cb) return ca < cb ? -1 : 1;
    }
    return a.size() == b.size() ? 0 : (a.size() < b.size() ? -1 : 1);
}

template <typename Map>
typename Map::mapped_type lookup(const Map& map, const typename Map::key_type& key) {
    const auto it = map.find(key);
    return it == map.end() ? typename Map::mapped_type{} : it->second;
}

// Numbered tracks sort before unnumbered ones, then by number, then by insertion.
bool positionLess(const SPTrack& a, const SPTrack& b) {
    const bool aNumbered = a.number > 0, bNumbered = b.number > 0;
    if (aNumbered != bNumbered) return aNumbered;
    if (a.number != b.number) return a.number < b.number;
    return a.id < b.id;
}

} // namespace

// ── Loading ───────────────────────────────────────────────────────────────────

MusicEntityRepository::MusicEntityRepository(Database& db) {
    std::unordered_map<int64_t, int64_t> playableByTrack;
    std::unordered_map<int64_t, std::string> uriByPlayable;
    try {
        SPDBConnection conn(db);
        m_artists          = SPArtist::findAll(conn);
        m_recordings       = SPRecording::findAll(conn);
        m_releases         = SPRelease::findAll(conn);
        m_tracks           = SPTrack::findAll(conn);
        m_recordingArtists = SPRecordingArtist::findAll(conn);
        m_releaseArtists   = SPReleaseArtist::findAll(conn);
        for (const auto& playable : SPPlayable::findAll(conn))
            uriByPlayable[playable.id] = playable.uri;
        // A track plays through its first playable.
        for (const auto& link : SPPlayableTrack::findAll(conn)) {
            auto [it, inserted] = playableByTrack.emplace(link.trackId, link.playableId);
            if (!inserted && link.playableId < it->second) it->second = link.playableId;
        }
        for (const auto& save : SPSave::findActive(conn))
            m_savedTracks.insert(save.trackId);
    } catch (const std::exception&) {
        m_artists.clear();
        m_recordings.clear();
        m_releases.clear();
        m_tracks.clear();
        m_recordingArtists.clear();
        m_releaseArtists.clear();
        m_savedTracks.clear();
        return;
    }

    for (const auto& artist : m_artists)       m_artistByIsni[artist.isni] = &artist;
    for (const auto& recording : m_recordings) m_recordingByIsrc[recording.isrc] = &recording;
    for (const auto& release : m_releases)     m_releaseByUpc[release.upc] = &release;
    for (const auto& [trackId, playableId] : playableByTrack) {
        const auto uri = uriByPlayable.find(playableId);
        if (uri != uriByPlayable.end()) m_playUriByTrack[trackId] = uri->second;
    }

    // Credit order: by number, then by artist name.
    const auto creditLess = [this](const std::string& artistA, int numberA,
                                   const std::string& artistB, int numberB) {
        if (numberA != numberB) return numberA < numberB;
        const SPArtist* a = lookup(m_artistByIsni, artistA);
        const SPArtist* b = lookup(m_artistByIsni, artistB);
        return (a ? a->name : std::string()) < (b ? b->name : std::string());
    };
    std::stable_sort(m_recordingArtists.begin(), m_recordingArtists.end(),
        [&](const SPRecordingArtist& a, const SPRecordingArtist& b) {
            return creditLess(a.artistId, a.number, b.artistId, b.number);
        });
    std::stable_sort(m_releaseArtists.begin(), m_releaseArtists.end(),
        [&](const SPReleaseArtist& a, const SPReleaseArtist& b) {
            return creditLess(a.artistId, a.number, b.artistId, b.number);
        });
}

// ── Building entities ─────────────────────────────────────────────────────────

std::vector<SPArtist> MusicEntityRepository::creditsOf(const std::string& key, bool release) const {
    std::vector<SPArtist> artists;
    const auto add = [&](const std::string& artistId) {
        if (const SPArtist* artist = lookup(m_artistByIsni, artistId)) artists.push_back(*artist);
    };
    if (release) {
        for (const auto& credit : m_releaseArtists)
            if (credit.releaseId == key) add(credit.artistId);
    } else {
        for (const auto& credit : m_recordingArtists)
            if (credit.recordingId == key) add(credit.artistId);
    }
    return artists;
}

const SPRelease* MusicEntityRepository::releaseOf(const SPTrack& track) const {
    return track.releaseId.empty() ? nullptr : lookup(m_releaseByUpc, track.releaseId);
}

std::shared_ptr<const ReleaseEntity> MusicEntityRepository::makeRelease(const SPRelease& release) const {
    const int trackCount = static_cast<int>(std::count_if(m_tracks.begin(), m_tracks.end(),
        [&](const SPTrack& track) { return track.releaseId == release.upc; }));
    return std::make_shared<ReleaseEntity>(release, creditsOf(release.upc, true), trackCount);
}

MusicEntityRepository::Tracks MusicEntityRepository::makeTracks(std::vector<const SPTrack*> tracks) const {
    // Group the credits by recording once, rather than scanning them per track.
    std::unordered_map<std::string, std::vector<SPArtist>> credits;
    for (const auto& credit : m_recordingArtists)
        if (const SPArtist* artist = lookup(m_artistByIsni, credit.artistId))
            credits[credit.recordingId].push_back(*artist);

    Tracks out;
    out.reserve(tracks.size());
    for (const SPTrack* track : tracks) {
        const SPRecording* recording = lookup(m_recordingByIsrc, track->recordingId);
        if (!recording) continue;
        const SPRelease* release = releaseOf(*track);
        out.push_back(std::make_shared<TrackEntity>(
            *track, *recording, release ? std::optional<SPRelease>(*release) : std::nullopt,
            lookup(credits, recording->isrc), lookup(m_playUriByTrack, track->id),
            m_savedTracks.count(track->id) != 0));
    }
    return out;
}

// ── Single lookups ────────────────────────────────────────────────────────────

std::shared_ptr<const ArtistEntity> MusicEntityRepository::artist(const std::string& isni) const {
    const SPArtist* artist = lookup(m_artistByIsni, isni);
    if (!artist) return nullptr;
    const int recordings = static_cast<int>(std::count_if(
        m_recordingArtists.begin(), m_recordingArtists.end(),
        [&](const SPRecordingArtist& credit) { return credit.artistId == isni; }));
    const int releases = static_cast<int>(std::count_if(
        m_releaseArtists.begin(), m_releaseArtists.end(),
        [&](const SPReleaseArtist& credit) { return credit.artistId == isni; }));
    return std::make_shared<ArtistEntity>(*artist, recordings, releases);
}

std::shared_ptr<const RecordingEntity> MusicEntityRepository::recording(const std::string& isrc) const {
    const SPRecording* recording = lookup(m_recordingByIsrc, isrc);
    if (!recording) return nullptr;
    // A recording plays through the first of its tracks that has a playable.
    for (const auto& track : tracksForRecording(isrc))
        if (!track->playUri().empty())
            return std::make_shared<RecordingEntity>(*recording, creditsOf(isrc, false),
                                                     track->trackId(), track->playUri());
    return std::make_shared<RecordingEntity>(*recording, creditsOf(isrc, false), 0, "");
}

std::shared_ptr<const ReleaseEntity> MusicEntityRepository::release(const std::string& upc) const {
    const SPRelease* release = lookup(m_releaseByUpc, upc);
    return release ? makeRelease(*release) : nullptr;
}

// ── Browse lists ──────────────────────────────────────────────────────────────

MusicEntityRepository::Artists MusicEntityRepository::artists() const {
    std::unordered_map<std::string, int> recordingCount, releaseCount;
    for (const auto& credit : m_recordingArtists) ++recordingCount[credit.artistId];
    for (const auto& credit : m_releaseArtists)   ++releaseCount[credit.artistId];

    std::vector<const SPArtist*> sorted;
    for (const auto& artist : m_artists) sorted.push_back(&artist);
    std::stable_sort(sorted.begin(), sorted.end(), [](const SPArtist* a, const SPArtist* b) {
        return compareNoCase(a->name, b->name) < 0;
    });

    Artists out;
    out.reserve(sorted.size());
    for (const SPArtist* artist : sorted)
        out.push_back(std::make_shared<ArtistEntity>(
            *artist, lookup(recordingCount, artist->isni), lookup(releaseCount, artist->isni)));
    return out;
}

MusicEntityRepository::Recordings MusicEntityRepository::recordings() const {
    // A recording plays through the first of its tracks that has a playable.
    std::unordered_map<std::string, std::pair<int64_t, std::string>> playThrough;
    for (const auto& track : tracks())
        if (!track->playUri().empty())
            playThrough.emplace(track->recording().isrc,
                                std::make_pair(track->trackId(), track->playUri()));

    std::unordered_map<std::string, std::vector<SPArtist>> credits;
    for (const auto& credit : m_recordingArtists)
        if (const SPArtist* artist = lookup(m_artistByIsni, credit.artistId))
            credits[credit.recordingId].push_back(*artist);

    std::vector<const SPRecording*> sorted;
    for (const auto& recording : m_recordings) sorted.push_back(&recording);
    std::stable_sort(sorted.begin(), sorted.end(), [](const SPRecording* a, const SPRecording* b) {
        return compareNoCase(a->name, b->name) < 0;
    });

    Recordings out;
    out.reserve(sorted.size());
    for (const SPRecording* recording : sorted) {
        const auto play = lookup(playThrough, recording->isrc);
        out.push_back(std::make_shared<RecordingEntity>(
            *recording, lookup(credits, recording->isrc), play.first, play.second));
    }
    return out;
}

MusicEntityRepository::Releases MusicEntityRepository::releases() const {
    std::unordered_map<std::string, int> trackCount;
    for (const auto& track : m_tracks)
        if (!track.releaseId.empty()) ++trackCount[track.releaseId];
    std::unordered_map<std::string, std::vector<SPArtist>> credits;
    for (const auto& credit : m_releaseArtists)
        if (const SPArtist* artist = lookup(m_artistByIsni, credit.artistId))
            credits[credit.releaseId].push_back(*artist);

    std::vector<const SPRelease*> sorted;
    for (const auto& release : m_releases) sorted.push_back(&release);
    std::stable_sort(sorted.begin(), sorted.end(), [](const SPRelease* a, const SPRelease* b) {
        return compareNoCase(a->name, b->name) < 0;
    });

    Releases out;
    out.reserve(sorted.size());
    for (const SPRelease* release : sorted)
        out.push_back(std::make_shared<ReleaseEntity>(
            *release, lookup(credits, release->upc), lookup(trackCount, release->upc)));
    return out;
}

// ── Track lists ───────────────────────────────────────────────────────────────

MusicEntityRepository::Tracks MusicEntityRepository::tracks() const {
    std::vector<const SPTrack*> sorted;
    for (const auto& track : m_tracks) sorted.push_back(&track);
    std::stable_sort(sorted.begin(), sorted.end(), [this](const SPTrack* a, const SPTrack* b) {
        const SPRelease* ra = releaseOf(*a);
        const SPRelease* rb = releaseOf(*b);
        if (!ra != !rb) return ra != nullptr; // tracks without a release last
        if (ra && rb)
            if (const int byName = compareNoCase(ra->name, rb->name)) return byName < 0;
        return positionLess(*a, *b);
    });
    return makeTracks(std::move(sorted));
}

MusicEntityRepository::Tracks MusicEntityRepository::tracksForRelease(const std::string& upc) const {
    std::vector<const SPTrack*> sorted;
    for (const auto& track : m_tracks)
        if (track.releaseId == upc) sorted.push_back(&track);
    std::stable_sort(sorted.begin(), sorted.end(),
                     [](const SPTrack* a, const SPTrack* b) { return positionLess(*a, *b); });
    return makeTracks(std::move(sorted));
}

MusicEntityRepository::Tracks MusicEntityRepository::tracksForArtist(const std::string& isni) const {
    std::unordered_set<std::string> credited;
    for (const auto& credit : m_recordingArtists)
        if (credit.artistId == isni) credited.insert(credit.recordingId);

    std::vector<const SPTrack*> sorted;
    for (const auto& track : m_tracks)
        if (credited.count(track.recordingId)) sorted.push_back(&track);
    std::stable_sort(sorted.begin(), sorted.end(), [this](const SPTrack* a, const SPTrack* b) {
        const SPRecording* ca = lookup(m_recordingByIsrc, a->recordingId);
        const SPRecording* cb = lookup(m_recordingByIsrc, b->recordingId);
        if (ca && cb)
            if (const int byName = compareNoCase(ca->name, cb->name)) return byName < 0;
        const SPRelease* ra = releaseOf(*a);
        const SPRelease* rb = releaseOf(*b);
        if (!ra != !rb) return ra == nullptr; // tracks without a release first
        if (ra && rb)
            if (const int byRelease = compareNoCase(ra->name, rb->name)) return byRelease < 0;
        return a->id < b->id;
    });
    return makeTracks(std::move(sorted));
}

MusicEntityRepository::Tracks MusicEntityRepository::tracksForRecording(const std::string& isrc) const {
    std::vector<const SPTrack*> sorted;
    for (const auto& track : m_tracks)
        if (track.recordingId == isrc) sorted.push_back(&track);
    std::stable_sort(sorted.begin(), sorted.end(), [this](const SPTrack* a, const SPTrack* b) {
        const SPRelease* ra = releaseOf(*a);
        const SPRelease* rb = releaseOf(*b);
        if (!ra != !rb) return ra == nullptr; // tracks without a release first
        if (ra && rb) {
            if (ra->releaseDate != rb->releaseDate) return ra->releaseDate < rb->releaseDate;
            if (const int byName = compareNoCase(ra->name, rb->name)) return byName < 0;
        }
        return a->id < b->id;
    });
    return makeTracks(std::move(sorted));
}

MusicEntityRepository::Releases MusicEntityRepository::releasesForArtist(const std::string& isni) const {
    std::unordered_set<std::string> upcs;
    for (const auto& credit : m_releaseArtists)
        if (credit.artistId == isni) upcs.insert(credit.releaseId);
    std::unordered_set<std::string> credited;
    for (const auto& credit : m_recordingArtists)
        if (credit.artistId == isni) credited.insert(credit.recordingId);
    for (const auto& track : m_tracks)
        if (!track.releaseId.empty() && credited.count(track.recordingId))
            upcs.insert(track.releaseId);

    std::vector<const SPRelease*> sorted;
    for (const auto& release : m_releases)
        if (upcs.count(release.upc)) sorted.push_back(&release);
    std::stable_sort(sorted.begin(), sorted.end(), [](const SPRelease* a, const SPRelease* b) {
        if (a->releaseDate != b->releaseDate) return a->releaseDate < b->releaseDate;
        return compareNoCase(a->name, b->name) < 0;
    });

    Releases out;
    out.reserve(sorted.size());
    for (const SPRelease* release : sorted) out.push_back(makeRelease(*release));
    return out;
}

} // namespace space
