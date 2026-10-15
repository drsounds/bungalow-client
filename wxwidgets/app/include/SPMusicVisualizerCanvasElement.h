#pragma once

#include "SPElement.h"
#include "SPPlayerCanvasImplementationElement.h"
#include <SPMusicVisualizerElement.h>

namespace space {
namespace views {

/// SPPlayerCanvasImplementationElement that hosts an SPMusicVisualizerElement.
///
/// SPMusicVisualizerElement is left freestanding (it can be used anywhere).
/// This class is the thin canvas-slot adapter: it embeds one as a flex child
/// and wires the SPPlayerCanvasImplementationElement lifecycle into it.
class SPMusicVisualizerCanvasElement
    : public SPElement
    , public SPPlayerCanvasImplementationElement
{
public:
    explicit SPMusicVisualizerCanvasElement(SPElement* parent);

    // ── SPElement ─────────────────────────────────────────────────────────────
    std::string getType() override { return "visualizercanvas"; }

    // ── SPPlayerCanvasImplementationElement ───────────────────────────────────
    std::string getName()    const override { return "Visualizer"; }
    SPElement*  asElement()        override { return this; }

    void onActivated()   override;
    void onDeactivated() override;

    /// onAudioFrame is a no-op here: SPMusicVisualizerElement fetches PCM/FFT
    /// data on its own render timer.  The canvas-element audio timer is used
    /// only by web-view-based implementations.
    void onAudioFrame(const float*, int, const float*, int) override {}

    void onPlaybackChanged(bool playing, int posMs, int durMs) override;

    std::vector<SPVisualizationParam>* getVisualizationParams() override;

    // ── Visualizer access ─────────────────────────────────────────────────────

    /// Swap the active SPVisualization.  Forwarded to the embedded element.
    void setVisualization(SPVisualization* viz);

    SPMusicVisualizerElement* getVisualizerElement() const { return m_visualizer; }

private:
    SPMusicVisualizerElement* m_visualizer = nullptr;
};

} // namespace views
} // namespace space
