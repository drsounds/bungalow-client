#pragma once

#include "service/MusicService.h"

#include <string>

class Database;

namespace space {
namespace service {

/// Plays local audio files (file:// URIs: MP3, AIFF, M4A, WAV) and resolves
/// tracks against the library database, so a library track with a local file
/// is one of the matches the PlaybackCoordinator gathers.
///
/// Audio goes through the sp_audio_* bridge: WinAudio.cpp (Media Foundation)
/// on Windows, AVAudioPlayerBridge.mm (AVAudioEngine) on macOS.
class LocalMediaService : public MusicService {
public:
    /// @p database may be null, in which case lookups find nothing; it must
    /// outlive the service otherwise.
    explicit LocalMediaService(Database* database = nullptr) : m_database(database) {}

    std::string getId() const override   { return "local"; }
    std::string getName() const override { return "Local Files"; }

    // ── MediaService ───────────────────────────────────────────────────────

    /// True for any URI that starts with "file://".
    bool acceptsUri(const std::string& uri) const override;

    void loadUri(const std::string& uri) override;
    void play()  override;
    void pause() override;
    void stop()  override;
    void seek(int positionMs) override;

    int getDurationMs() const override;
    int getPositionMs() const override;

    // ── MusicService ───────────────────────────────────────────────────────

    /// A library track of this recording that has a local file.
    std::optional<Candidate> lookup(const std::string& isrc) override;
    /// A library track with this title (any case) and, when given, an artist
    /// credit containing @p artist, that has a local file.
    std::optional<Candidate> lookupByTitleArtist(const std::string& title,
                                                 const std::string& artist) override;

private:
    Database*   m_database;
    std::string m_loadedUri;
};

} // namespace service
} // namespace space
