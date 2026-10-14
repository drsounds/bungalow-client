/// SPMusicVisualizerCanvasElement.cpp
/// Canvas-slot adapter that wraps SPMusicVisualizerElement.

#include "SPMusicVisualizerCanvasElement.h"

namespace space {
namespace views {

SPMusicVisualizerCanvasElement::SPMusicVisualizerCanvasElement(SPElement* parent)
    : SPElement(parent)
{
    setTagName("visualizercanvas");
    setBackgroundColor(SPColor(0, 0, 0, 255));

    m_visualizer = new SPMusicVisualizerElement(this);
    m_visualizer->setFlex(1);
    appendChild(m_visualizer);
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void SPMusicVisualizerCanvasElement::onActivated() {
    setVisible(true);
    if (m_visualizer) m_visualizer->setVisible(true);
}

void SPMusicVisualizerCanvasElement::onDeactivated() {
    if (m_visualizer) m_visualizer->setVisible(false);
    setVisible(false);
}

void SPMusicVisualizerCanvasElement::onPlaybackChanged(bool playing,
                                                        int  posMs,
                                                        int  durMs) {
    // SPMusicVisualizerElement subscribes to playback events itself, so nothing
    // extra is needed here.  The override is provided for completeness and to
    // allow subclasses to act on this hook without touching the embedded element.
    (void)playing; (void)posMs; (void)durMs;
}

// ── Params ────────────────────────────────────────────────────────────────────

std::vector<SPVisualizationParam>*
SPMusicVisualizerCanvasElement::getVisualizationParams() {
    if (!m_visualizer) return nullptr;
    SPVisualization* viz = m_visualizer->getVisualization();
    if (!viz) return nullptr;
    return &viz->params;
}

// ── Visualizer management ─────────────────────────────────────────────────────

void SPMusicVisualizerCanvasElement::setVisualization(SPVisualization* viz) {
    if (m_visualizer)
        m_visualizer->setVisualization(viz);
}

} // namespace views
} // namespace space
