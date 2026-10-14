#pragma once

#include "SPVBoxElement.h"
#include "SPPlayerCanvasImplementationElement.h"

#include <wx/timer.h>

namespace space {
namespace views {

class CanvasAudioTimer;

/// Container element that acts as a runtime slot for SPPlayerCanvasImplementationElement.
///
/// The active implementation occupies the entire element (flex = 1).
/// Switching implementations is done via setImplementation(); the old
/// implementation receives onDeactivated() and is removed from the DOM,
/// the new one is inserted and receives onActivated().
///
/// SPPlayerCanvasElement also:
///   - Subscribes to "playback:changed" and forwards the event to the
///     active implementation via onPlaybackChanged().
///   - Runs a 30-fps audio timer while playing that reads PCM + FFT data
///     and calls onAudioFrame() on the active implementation.
class SPPlayerCanvasElement : public SPVBoxElement {
public:
    explicit SPPlayerCanvasElement(SPElement* parent);
    ~SPPlayerCanvasElement() override;

    std::string getType() override { return "playercanvas"; }

    // ── Implementation slot ───────────────────────────────────────────────────

    /// Swap the active canvas implementation.
    /// Dispatches "implementationChanged" after the swap so listeners
    /// (e.g. PlayerView's parameter panel) can react.
    void setImplementation(SPPlayerCanvasImplementationElement* impl);

    SPPlayerCanvasImplementationElement* getImplementation() const { return m_current; }

    // ── Audio timer ───────────────────────────────────────────────────────────

    /// Frames per second at which onAudioFrame() is called on the active
    /// implementation (default: 30).
    void setAudioFps(int fps);
    int  getAudioFps() const { return m_audioFps; }

    /// Called by CanvasAudioTimer::Notify() — public for timer access.
    void onAudioTimerFired();

private:
    SPPlayerCanvasImplementationElement* m_current  = nullptr;
    CanvasAudioTimer*                    m_audioTimer = nullptr;
    int                                  m_audioFps   = 30;

    int onPlaybackChanged(SPObject* sender, SPEventArgs* args);
};

} // namespace views
} // namespace space
