/// SPPlayerCanvasElement.cpp
/// Runtime slot for SPPlayerCanvasImplementationElement subclasses.

#include "SPPlayerCanvasElement.h"
#include "SPPlaybackEventArgs.h"
#include "BungalowApp.h"

#include <wx/app.h>
#include <wx/timer.h>

// ── C bridge (AVAudioPlayerBridge.mm) ────────────────────────────────────────

extern "C" {
    int sp_audio_get_pcm(float* outBuffer, int maxCount);
    int sp_audio_get_fft(float* outBuffer, int maxCount);
}

namespace space {
namespace views {

// ── CanvasAudioTimer ──────────────────────────────────────────────────────────

class CanvasAudioTimer : public wxTimer {
    SPPlayerCanvasElement* m_owner;
public:
    explicit CanvasAudioTimer(SPPlayerCanvasElement* owner) : m_owner(owner) {}
    void Notify() override { m_owner->onAudioTimerFired(); }
};

// ── Construction / destruction ────────────────────────────────────────────────

SPPlayerCanvasElement::SPPlayerCanvasElement(SPElement* parent)
    : SPVBoxElement(parent)
{
    setTagName("playercanvas");
    m_audioTimer = new CanvasAudioTimer(this);

    if (auto* app = dynamic_cast<BungalowApp*>(wxTheApp)) {
        app->getPlaybackEvents().addEventListener(
            "playback:changed",
            reinterpret_cast<SPCallback>(&SPPlayerCanvasElement::onPlaybackChanged),
            this,
            nullptr);
    }
}

SPPlayerCanvasElement::~SPPlayerCanvasElement() {
    if (m_audioTimer) {
        m_audioTimer->Stop();
        delete m_audioTimer;
    }
    if (m_current)
        m_current->onDeactivated();
}

// ── Implementation slot ───────────────────────────────────────────────────────

void SPPlayerCanvasElement::setImplementation(SPPlayerCanvasImplementationElement* impl) {
    // Deactivate and remove the outgoing implementation.
    if (m_current) {
        m_current->onDeactivated();
        removeChild(m_current->asElement());
    }

    m_current = impl;

    if (m_current) {
        SPElement* el = m_current->asElement();
        el->setFlex(1);
        appendChild(el);
        m_current->onActivated();
        pack();
        invalidate();
    }

    // Notify listeners (e.g. PlayerView's parameter panel).
    notify("implementationChanged", this, nullptr);
}

// ── Audio timer ───────────────────────────────────────────────────────────────

void SPPlayerCanvasElement::setAudioFps(int fps) {
    m_audioFps = fps;
    if (m_audioTimer->IsRunning())
        m_audioTimer->Start(1000 / m_audioFps);
}

void SPPlayerCanvasElement::onAudioTimerFired() {
    if (!m_current) return;

    static const int kMaxPcm = 1024;
    static const int kMaxFft = 512;
    float pcm[kMaxPcm] = {};
    float fft[kMaxFft] = {};
    int pcmN = sp_audio_get_pcm(pcm, kMaxPcm);
    int fftN = sp_audio_get_fft(fft, kMaxFft);

    m_current->onAudioFrame(pcm, pcmN, fft, fftN);
}

// ── Playback events ───────────────────────────────────────────────────────────

int SPPlayerCanvasElement::onPlaybackChanged(SPObject* /*sender*/,
                                              SPEventArgs* args) {
    auto* pba = dynamic_cast<SPPlaybackStateEventArgs*>(args);
    if (!pba) return 0;

    if (pba->playing) {
        if (!m_audioTimer->IsRunning())
            m_audioTimer->Start(1000 / m_audioFps);
    } else {
        if (m_audioTimer->IsRunning())
            m_audioTimer->Stop();
        // Send a final silent frame so implementations can clear their display.
        if (m_current) {
            float zeros[1] = {};
            m_current->onAudioFrame(zeros, 0, zeros, 0);
        }
    }

    if (m_current)
        m_current->onPlaybackChanged(pba->playing, pba->positionMs, pba->durationMs);

    return 0;
}

} // namespace views
} // namespace space
