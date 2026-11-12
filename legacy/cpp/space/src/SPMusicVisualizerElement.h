#pragma once

#include "SPElement.h"
#include "SPVisualization.h"

#include <wx/glcanvas.h>
#include <wx/timer.h>

namespace space {
namespace views {

class RenderTimerHelper;

/// An SPElement that renders an SPVisualization using OpenGL via wxGLCanvas.
///
/// The canvas is created lazily on the first pack() call where absoluteBounds
/// is non-zero, then repositioned/resized on every subsequent pack() — exactly
/// the same lifecycle as the wxWebView inside SiteView.
///
/// Audio data (PCM + FFT) is fetched each frame from the C bridge functions
/// declared in AVAudioPlayerBridge.mm.
///
/// PlayerView owns the active visualization and the parameter panel; this
/// element only concerns itself with GL rendering and the render timer.
class SPMusicVisualizerElement : public SPElement {
public:
    explicit SPMusicVisualizerElement(SPElement* parent);
    ~SPMusicVisualizerElement() override;

    std::string getType() override { return "musicvisualizer"; }

    void pack()                                     override;
    void paint(int x, int y, SPGraphicsContext* gc) override;
    void setVisible(bool value)                     override;

    /// Swap the active visualization.
    /// Dispatches a "visualizationChanged" event so PlayerView can rebuild
    /// its parameter panel.
    void             setVisualization(SPVisualization* viz);
    SPVisualization* getVisualization() const { return m_visualization; }

    /// Render-timer frequency (default: 60 fps).
    void setFps(int fps);
    int  getFps() const { return m_fps; }

    /// Called by RenderTimerHelper::Notify() — public to allow friend access.
    void onRenderTimerFired();

private:
    SPVisualization* m_visualization = nullptr;
    wxGLCanvas*      m_glCanvas      = nullptr;
    wxGLContext*     m_glContext      = nullptr;
    RenderTimerHelper* m_renderTimer = nullptr;
    int              m_fps           = 60;
    bool             m_glInitialized = false;

    void ensureGLCanvas();
    void positionGLCanvas();
    void initGL();
    void renderFrame();

    int onPlaybackChanged(SPObject* sender, SPEventArgs* args);
};

} // namespace views
} // namespace space
