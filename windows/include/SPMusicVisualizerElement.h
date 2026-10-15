#pragma once

#include "SPElement.h"
#include "SPVisualization.h"

namespace space {
namespace views {

/// Win32 stand-in for the OpenGL music visualizer. It keeps the wx element's API
/// but renders a placeholder, because the GL canvas and PCM/FFT capture are
/// not available in this backend.
class SPMusicVisualizerElement : public SPElement {
public:
    explicit SPMusicVisualizerElement(SPElement* parent);
    ~SPMusicVisualizerElement() override;

    std::string getType() override { return "musicvisualizer"; }

    void paint(int x, int y, SPGraphicsContext* gc) override;

    void             setVisualization(SPVisualization* viz);
    SPVisualization* getVisualization() const { return m_visualization; }

    void setFps(int fps) { m_fps = fps; }
    int  getFps() const { return m_fps; }

private:
    SPVisualization* m_visualization = nullptr;
    int              m_fps           = 60;
};

} // namespace views
} // namespace space
