#pragma once

#include "service/MusicService.h"

#include <memory>
#include <mutex>
#include <string>

class Database;

namespace space {
namespace service {

/// Plays local audio files (file:// URIs: MP3, AIFF, M4A, WAV) and resolves
/// tracks against the library database, so a library track with a local file
/// is one of the matches the PlaybackCoordinator gathers.
///
/// Audio goes through the sp_audio_* bridge: WinAudio.cpp (Media Foundation)
/// on Windows, AVAudioPlayerBridge.mm (AVAudioEngine) on macOS. Transport
/// calls belong on the UI thread; lookups may run on any thread.
class LocalMediaService : public MusicService {
public:
    static constexpr const char* kId = "local";

    /// Lookups read the library at @p databasePath through their own
    /// read-only connection, so they can run beside the UI thread's writes.
    /// With an empty path, or one that can't be opened, they find nothing.
    explicit LocalMediaService(const std::string& databasePath = {});
    ~LocalMediaService() override;

    std::string getId() const override   { return kId; }
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
    std::unique_ptr<Database> m_lookupDb;
    std::mutex                m_lookupMutex; // the connection is used by one thread at a time
    std::string               m_loadedUri;
};

} // namespace service
} // namespace space
