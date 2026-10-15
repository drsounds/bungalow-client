#pragma once

#include "SPService.h"
#include <string>

/// Base class for services that provide audio/media playback.
///
/// Derives from SPService and adds the transport-control interface that all
/// concrete media backends (local files, Spotify, Tidal, …) must implement.
class SPMediaService : public SPService {
public:
    /// Begin or resume playback of the currently loaded URI.
    virtual void play() = 0;

    /// Pause playback without discarding the current position.
    virtual void pause() = 0;

    /// Load a URI and prepare it for playback. Does not start playing.
    /// @param uri  The resource to load (e.g. "file:///path/to/track.mp3").
    virtual void load(const std::string& uri) = 0;

    /// Stop playback and reset the playback position to zero.
    virtual void stop() = 0;

    /// Seek to the given position within the current track.
    /// @param pos  Position in milliseconds from the start.
    virtual void seek(int pos) = 0;

    /// Returns the duration of the currently loaded track in milliseconds.
    /// Returns 0 if unknown or no track is loaded.
    virtual int getDurationMs() const { return 0; }

    /// Returns the current playback position in milliseconds.
    /// Returns 0 if unknown or no track is loaded.
    virtual int getPositionMs() const { return 0; }
};
