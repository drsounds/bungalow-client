/// SPMusicVisualizerElement.cpp
/// OpenGL-backed music visualizer element.

#include "SPMusicVisualizerElement.h"
#include "SPPlaybackEventArgs.h"
#include "SPGraphicsContext.h"
#include "WxWidgetsGraphicsContext.h"
#include "SpaceWidget.h"
#include "SpacifyApp.h"

#include <wx/app.h>

#ifdef __WXMAC__
#  include <OpenGL/gl.h>
#endif

// ── C bridge (AVAudioPlayerBridge.mm) ────────────────────────────────────────

extern "C" {
    /// Copy the latest PCM frame into outBuffer (mono floats, range −1…1).
    /// Returns the number of samples actually written (≤ maxCount).
    int sp_audio_get_pcm(float* outBuffer, int maxCount);

    /// Copy the latest FFT magnitude frame into outBuffer.
    /// Returns the number of bins actually written (≤ maxCount).
    int sp_audio_get_fft(float* outBuffer, int maxCount);
}

namespace space {
namespace views {

// ── RenderTimerHelper ─────────────────────────────────────────────────────────

/// Thin wxTimer subclass that forwards Notify() back to the element.
/// Defined here (in the .cpp) so the header stays wx-agnostic.
class RenderTimerHelper : public wxTimer {
    SPMusicVisualizerElement* m_owner;
public:
    explicit RenderTimerHelper(SPMusicVisualizerElement* owner) : m_owner(owner) {}
    void Notify() override { m_owner->onRenderTimerFired(); }
};

// ── Construction / destruction ────────────────────────────────────────────────

SPMusicVisualizerElement::SPMusicVisualizerElement(SPElement* parent)
    : SPElement(parent)
{
    setTagName("musicvisualizer");
    setBackgroundColor(SPColor(0, 0, 0, 255));

    m_renderTimer = new RenderTimerHelper(this);

    if (auto* app = dynamic_cast<SpacifyApp*>(wxTheApp)) {
        app->getPlaybackEvents().addEventListener(
            "playback:changed",
            reinterpret_cast<SPCallback>(&SPMusicVisualizerElement::onPlaybackChanged),
            this,
            nullptr);
    }
}

SPMusicVisualizerElement::~SPMusicVisualizerElement() {
    if (m_renderTimer) {
        m_renderTimer->Stop();
        delete m_renderTimer;
    }
    delete m_glContext;
    delete m_glCanvas;
}

// ── Visualization management ──────────────────────────────────────────────────

void SPMusicVisualizerElement::setVisualization(SPVisualization* viz) {
    m_visualization  = viz;
    m_glInitialized  = false;   // force GL re-init for new visualization
    notify("visualizationChanged", this, nullptr);
}

void SPMusicVisualizerElement::setFps(int fps) {
    m_fps = fps;
    if (m_renderTimer->IsRunning())
        m_renderTimer->Start(1000 / m_fps);
}

// ── Layout ────────────────────────────────────────────────────────────────────

void SPMusicVisualizerElement::pack() {
    SPElement::pack();

    if (!m_glCanvas)
        ensureGLCanvas();
    else
        positionGLCanvas();
}

void SPMusicVisualizerElement::paint(int x, int y, SPGraphicsContext* gc) {
    // Draw the black background so something is visible before the GL canvas
    // appears or if no visualization is set.
    SPElement::paint(x, y, gc);

    // Second attempt to create the canvas in case pack() ran before layout was
    // ready (absoluteBounds was zero at pack() time).
    if (!m_glCanvas)
        ensureGLCanvas();
}

// ── GL canvas lifecycle ───────────────────────────────────────────────────────

void SPMusicVisualizerElement::ensureGLCanvas() {
    if (m_glCanvas) return;

    auto b = getAbsoluteBounds();
    if (b.width <= 0 || b.height <= 0) return;

    auto* gc = dynamic_cast<platform::wxwidgets::WxWidgetsGraphicsContext*>(
        createGraphics());
    if (!gc) return;

    SpaceWidget* widget = gc->getWidget();
    if (!widget) return;

    wxGLAttributes attrs;
    attrs.PlatformDefaults().RGBA().DoubleBuffer().Depth(16).EndList();

    m_glCanvas  = new wxGLCanvas(widget, attrs, wxID_ANY,
                                  wxPoint(b.x, b.y),
                                  wxSize(b.width, b.height));
    m_glContext = new wxGLContext(m_glCanvas);
    m_glCanvas->Show(isVisible());
}

void SPMusicVisualizerElement::positionGLCanvas() {
    if (!m_glCanvas) return;
    auto b = getAbsoluteBounds();
    m_glCanvas->SetPosition(wxPoint(b.x, b.y));
    m_glCanvas->SetSize(wxSize(b.width, b.height));
}

// ── GL initialisation ─────────────────────────────────────────────────────────

void SPMusicVisualizerElement::initGL() {
    auto b = getAbsoluteBounds();
    glViewport(0, 0, b.width, b.height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Coordinate system: (0,0) bottom-left, (w,h) top-right — conventional for GL.
    glOrtho(0.0, b.width, 0.0, b.height, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    m_glInitialized = true;
}

// ── Render loop ───────────────────────────────────────────────────────────────

void SPMusicVisualizerElement::onRenderTimerFired() {
    renderFrame();
}

void SPMusicVisualizerElement::renderFrame() {
    if (!m_glCanvas || !m_glContext || !m_visualization) return;
    if (!m_glCanvas->IsShownOnScreen()) return;

    m_glCanvas->SetCurrent(*m_glContext);

    auto b = getAbsoluteBounds();
    if (!m_glInitialized)
        initGL();

    // Keep viewport in sync with any size changes.
    glViewport(0, 0, b.width, b.height);

    // Fetch the latest audio data from the audio thread's shared buffers.
    static const int kMaxPcm = 1024;
    static const int kMaxFft = 512;
    float pcm[kMaxPcm] = {};
    float fft[kMaxFft] = {};
    int pcmN = sp_audio_get_pcm(pcm, kMaxPcm);
    int fftN = sp_audio_get_fft(fft, kMaxFft);

    m_visualization->renderFrame(pcm, pcmN, fft, fftN, b.width, b.height);

    m_glCanvas->SwapBuffers();
}

// ── Playback events ───────────────────────────────────────────────────────────

int SPMusicVisualizerElement::onPlaybackChanged(SPObject* /*sender*/,
                                                SPEventArgs* args) {
    auto* pba = dynamic_cast<SPPlaybackStateEventArgs*>(args);
    if (!pba) return 0;

    if (pba->playing) {
        if (!m_renderTimer->IsRunning())
            m_renderTimer->Start(1000 / m_fps);
    } else {
        if (m_renderTimer->IsRunning())
            m_renderTimer->Stop();
        // Render one final frame to clear the visualizer to its resting state.
        renderFrame();
    }
    return 0;
}

// ── Visibility ────────────────────────────────────────────────────────────────

void SPMusicVisualizerElement::setVisible(bool value) {
    SPElement::setVisible(value);
    if (m_glCanvas)
        m_glCanvas->Show(value);
    if (!value && m_renderTimer->IsRunning())
        m_renderTimer->Stop();
}

} // namespace views
} // namespace space
