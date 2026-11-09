#pragma once

#include "service/Service.h"

#include <string>
#include <vector>

namespace space {
namespace service {

/// Streaming / playback capability: transport control over one loaded URI.
/// Local files and any streaming back-end are just implementations; the
/// PlaybackCoordinator decides which one plays a given track.
class MediaService : public virtual Service {
public:
    enum class PlaybackState { Idle, Loading, Playing, Paused, Stopped, Error };

    /// Observer of one MediaService. Every callback has an empty default.
    class PlaybackListener {
    public:
        virtual ~PlaybackListener() = default;
        virtual void onStateChanged(PlaybackState /*state*/) {}
        virtual void onPositionChanged(int /*positionMs*/, int /*durationMs*/) {}
        virtual void onTrackChanged(const std::string& /*title*/, const std::string& /*artist*/,
                                    const std::string& /*album*/) {}
        virtual void onError(const std::string& /*message*/) {}
        /// The loaded track reached its natural end.
        virtual void onCompleted() {}
    };

    /// Whether loadUri() can play this URI (e.g. "file://…" for local files).
    virtual bool acceptsUri(const std::string& uri) const = 0;

    /// Load @p uri and prepare it for playback without starting it. A failure
    /// is reported with fireError(); a successful load leaves the Error state.
    virtual void loadUri(const std::string& uri) = 0;
    /// Begin or resume playback of the loaded URI.
    virtual void play() = 0;
    /// Pause, keeping the position.
    virtual void pause() = 0;
    /// Stop and rewind to the start.
    virtual void stop() = 0;
    virtual void seek(int positionMs) = 0;

    /// 0 when unknown or nothing is loaded.
    virtual int getPositionMs() const { return 0; }
    virtual int getDurationMs() const { return 0; }

    PlaybackState getPlaybackState() const { return m_state; }

    /// Listeners are not owned and must be removed before they are destroyed.
    void addPlaybackListener(PlaybackListener* listener);
    void removePlaybackListener(PlaybackListener* listener);

protected:
    /// Records the state and notifies listeners when it changed.
    void setPlaybackState(PlaybackState state);
    void firePositionChanged(int positionMs, int durationMs);
    void fireTrackChanged(const std::string& title, const std::string& artist,
                          const std::string& album);
    /// Moves to PlaybackState::Error and reports @p message.
    void fireError(const std::string& message);
    void fireCompleted();

private:
    PlaybackState m_state = PlaybackState::Idle;
    std::vector<PlaybackListener*> m_listeners;
};

} // namespace service
} // namespace space
