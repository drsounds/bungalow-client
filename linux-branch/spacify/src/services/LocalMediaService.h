#pragma once

#include "SPMediaService.h"
#include <string>

/// SPMediaService implementation for local audio files.
///
/// Accepts URIs with the "file://" scheme and supports MP3, AIFF, M4A, and
/// WAV containers.  Playback is handled by AVAudioPlayer on macOS via the
/// AVAudioPlayerBridge (spacify/src/services/AVAudioPlayerBridge.mm).
class LocalMediaService : public SPMediaService {
public:
    // ── SPService ──────────────────────────────────────────────────────────

    /// Returns true for any URI that starts with "file://".
    bool acceptsUri(const std::string& uri) const override;

    void startLogin() override {}
    void logout()     override {}

    std::string getId() const override   { return "local"; }
    std::string getName() const override { return "Local Media"; }

    bool        isLoggedIn()       const override { return true; }
    std::string getLoggedInUser()  const override { return ""; }

    // ── SPMediaService ─────────────────────────────────────────────────────

    void load(const std::string& uri) override;
    void play()  override;
    void pause() override;
    void stop()  override;
    void seek(int posMs) override;

    int getDurationMs() const override;
    int getPositionMs() const override;

private:
    std::string m_loadedUri;
    bool        m_playing = false;
};