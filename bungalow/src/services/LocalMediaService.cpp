#include "services/LocalMediaService.h"

// ── AVAudioPlayerBridge C declarations ───────────────────────────────────────
// Implemented in AVAudioPlayerBridge.mm; linked in on macOS.

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

// ── Helpers ───────────────────────────────────────────────────────────────────

/// Strip "file://" prefix from a URI to obtain a plain filesystem path.
/// "file:///Users/foo/bar.mp3" → "/Users/foo/bar.mp3"
static std::string uriToPath(const std::string& uri) {
    if (uri.rfind("file://", 0) == 0)
        return uri.substr(7); // leave the leading '/'
    return uri;
}

// ── SPService ─────────────────────────────────────────────────────────────────

bool LocalMediaService::acceptsUri(const std::string& uri) const {
    return uri.rfind("file://", 0) == 0;
}

// ── SPMediaService ────────────────────────────────────────────────────────────

void LocalMediaService::load(const std::string& uri) {
    m_loadedUri = uri;
    m_playing   = false;
    sp_audio_load(uriToPath(uri).c_str());
}

void LocalMediaService::play() {
    if (m_loadedUri.empty()) return;
    m_playing = true;
    sp_audio_play();
}

void LocalMediaService::pause() {
    m_playing = false;
    sp_audio_pause();
}

void LocalMediaService::stop() {
    m_playing = false;
    sp_audio_stop();
}

void LocalMediaService::seek(int posMs) {
    sp_audio_seek(posMs / 1000.0);
}

int LocalMediaService::getDurationMs() const {
    return static_cast<int>(sp_audio_get_duration() * 1000.0);
}

int LocalMediaService::getPositionMs() const {
    return static_cast<int>(sp_audio_get_position() * 1000.0);
}
