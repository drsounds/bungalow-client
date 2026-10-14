#pragma once

#include "SPElement.h"
#include "SPVisualization.h"

#include <string>
#include <vector>

namespace space {

/// Mixin interface for "now playing" canvas implementations.
///
/// Any class that can serve as the active view inside SPPlayerCanvasElement
/// must implement this interface.  Concrete classes inherit from both an
/// SPElement subclass (for DOM insertion / rendering) and this mixin (for
/// the lifecycle and audio-data hooks).
///
/// Implementations currently shipped:
///   - SPMusicVisualizerCanvasElement  — OpenGL music visualizer
///   - SPAppView                       — bare web view (no nav bar)
///
/// To add a new implementation:
///   1. Subclass SPElement (or a subclass) AND SPPlayerCanvasImplementationElement.
///   2. Implement getName() and asElement().
///   3. Override the lifecycle hooks you care about.
///   4. Register with PlayerView::setCanvasImplementation().
class SPPlayerCanvasImplementationElement {
public:
    virtual ~SPPlayerCanvasImplementationElement() = default;

    // ── Identity ──────────────────────────────────────────────────────────────

    /// Human-readable name shown in the canvas switcher UI.
    virtual std::string getName() const = 0;

    /// Return this object as an SPElement* so SPPlayerCanvasElement can
    /// insert it into the DOM tree.  Implementations return `this`.
    virtual SPElement* asElement() = 0;

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /// Called by SPPlayerCanvasElement after this implementation becomes active.
    virtual void onActivated() {}

    /// Called by SPPlayerCanvasElement just before another implementation
    /// replaces this one (or before the canvas is destroyed).
    virtual void onDeactivated() {}

    // ── Playback events ───────────────────────────────────────────────────────

    /// Forwarded from SPPlayerCanvasElement whenever playback state changes.
    /// @param playing    True while audio is playing.
    /// @param posMs      Current playback position in milliseconds.
    /// @param durMs      Track duration in milliseconds (0 if unknown).
    virtual void onPlaybackChanged(bool playing, int posMs, int durMs) {}

    // ── Audio data ────────────────────────────────────────────────────────────

    /// Called on each tick of SPPlayerCanvasElement's audio timer while playing.
    ///
    /// @param pcm      Mono PCM float samples (range −1…1).
    /// @param pcmCount Number of samples in pcm.
    /// @param fft      FFT magnitude bins (linear scale, DC…Nyquist).
    /// @param fftCount Number of bins in fft.
    virtual void onAudioFrame(const float* pcm, int pcmCount,
                               const float* fft, int fftCount) {}

    // ── Parameter panel ───────────────────────────────────────────────────────

    /// Return a pointer to this implementation's visualization parameter list
    /// so PlayerView can build its dynamic parameter panel.
    /// Return nullptr if this implementation exposes no user-tunable params.
    virtual std::vector<SPVisualizationParam>* getVisualizationParams() { return nullptr; }
};

} // namespace space
