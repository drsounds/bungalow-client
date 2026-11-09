#include "service/LocalMediaService.h"

#include "Database.h"

#include <algorithm>
#include <cctype>

// ── sp_audio_* bridge ─────────────────────────────────────────────────────────
// WinAudio.cpp on Windows, AVAudioPlayerBridge.mm on macOS.

extern "C" {
    bool   sp_audio_load(const char* path);
    void   sp_audio_play();
    void   sp_audio_pause();
    void   sp_audio_stop();
    void   sp_audio_seek(double seconds);
    double sp_audio_get_duration();
    double sp_audio_get_position();
    bool   sp_audio_is_playing();
}

namespace space {
namespace service {

namespace {

const std::string kFileScheme = "file://";

/// "file:///Users/foo/bar.mp3" → "/Users/foo/bar.mp3"
std::string uriToPath(const std::string& uri) {
    if (uri.rfind(kFileScheme, 0) == 0) return uri.substr(kFileScheme.size());
    return uri;
}

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string displayTitle(const Database::TrackRow& track) {
    return track.version.empty() ? track.name : track.name + " (" + track.version + ")";
}

} // namespace

// ── MediaService ──────────────────────────────────────────────────────────────

bool LocalMediaService::acceptsUri(const std::string& uri) const {
    return uri.rfind(kFileScheme, 0) == 0;
}

void LocalMediaService::loadUri(const std::string& uri) {
    m_loadedUri = uri;
    setPlaybackState(PlaybackState::Loading);
    if (!sp_audio_load(uriToPath(uri).c_str())) {
        m_loadedUri.clear();
        fireError("Couldn't open " + uriToPath(uri));
        return;
    }
    setPlaybackState(PlaybackState::Stopped);
}

void LocalMediaService::play() {
    if (m_loadedUri.empty()) return;
    sp_audio_play();
    setPlaybackState(PlaybackState::Playing);
}

void LocalMediaService::pause() {
    sp_audio_pause();
    setPlaybackState(PlaybackState::Paused);
}

void LocalMediaService::stop() {
    sp_audio_stop();
    setPlaybackState(PlaybackState::Stopped);
}

void LocalMediaService::seek(int positionMs) {
    sp_audio_seek(positionMs / 1000.0);
}

int LocalMediaService::getDurationMs() const {
    return static_cast<int>(sp_audio_get_duration() * 1000.0);
}

int LocalMediaService::getPositionMs() const {
    return static_cast<int>(sp_audio_get_position() * 1000.0);
}

// ── MusicService ──────────────────────────────────────────────────────────────

std::optional<MusicService::Candidate> LocalMediaService::lookup(const std::string& isrc) {
    if (!m_database || isrc.empty()) return std::nullopt;
    for (const auto& track : m_database->getTracksForRecording(isrc)) {
        if (acceptsUri(track.playUri))
            return Candidate{track.playUri, displayTitle(track), track.artists, true};
    }
    return std::nullopt;
}

std::optional<MusicService::Candidate> LocalMediaService::lookupByTitleArtist(const std::string& title,
                                                                              const std::string& artist) {
    if (!m_database || title.empty()) return std::nullopt;
    const std::string wantedArtist = lower(artist);
    for (const auto& track : m_database->getTracksByTitle(title)) {
        if (!acceptsUri(track.playUri)) continue;
        if (!wantedArtist.empty() && lower(track.artists).find(wantedArtist) == std::string::npos) continue;
        return Candidate{track.playUri, displayTitle(track), track.artists, false};
    }
    return std::nullopt;
}

} // namespace service
} // namespace space
