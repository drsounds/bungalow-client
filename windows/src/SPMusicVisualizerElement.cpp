#include "SPMusicVisualizerElement.h"

namespace space {
namespace views {

SPMusicVisualizerElement::SPMusicVisualizerElement(SPElement* parent)
    : SPElement(parent)
{
    setTagName("musicvisualizer");
    setBackgroundColor(SPColor(0, 0, 0, 255));
}

SPMusicVisualizerElement::~SPMusicVisualizerElement() = default;

void SPMusicVisualizerElement::paint(int x, int y, SPGraphicsContext* gc) {
    SPElement::paint(x, y, gc);
}

void SPMusicVisualizerElement::setVisualization(SPVisualization* viz) {
    m_visualization = viz;
    notify("visualizationChanged", this, nullptr);
}

} // namespace views
} // namespace space
