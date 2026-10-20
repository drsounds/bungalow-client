#include "SPPlayerCanvasElement.h"
#include "SPPlaybackEventArgs.h"
#include "BungalowApp.h"

#include <Windows.h>

#include <map>

extern "C" {
    int sp_audio_get_pcm(float* outBuffer, int maxCount);
    int sp_audio_get_fft(float* outBuffer, int maxCount);
}

namespace space {
namespace views {

namespace {
    std::map<UINT_PTR, SPPlayerCanvasElement*>& activeTimers() {
        static std::map<UINT_PTR, SPPlayerCanvasElement*> timers;
        return timers;
    }

    VOID CALLBACK canvasTimerProc(HWND, UINT, UINT_PTR id, DWORD) {
        auto it = activeTimers().find(id);
        if (it != activeTimers().end()) it->second->onAudioTimerFired();
    }
}

SPPlayerCanvasElement::SPPlayerCanvasElement(SPElement* parent)
    : SPVBoxElement(parent)
{
    setTagName("playercanvas");

    if (auto* app = BungalowApp::current()) {
        app->getPlaybackEvents().addEventListener(
            "playback:changed",
            reinterpret_cast<SPCallback>(&SPPlayerCanvasElement::onPlaybackChanged),
            this,
            nullptr);
    }
}

SPPlayerCanvasElement::~SPPlayerCanvasElement() {
    stopTimer();
    if (m_current)
        m_current->onDeactivated();
}

void SPPlayerCanvasElement::startTimer() {
    if (m_timerId) return;
    UINT_PTR id = SetTimer(nullptr, 0, static_cast<UINT>(1000 / m_audioFps), canvasTimerProc);
    if (!id) return;
    m_timerId = id;
    activeTimers()[id] = this;
}

void SPPlayerCanvasElement::stopTimer() {
    if (!m_timerId) return;
    KillTimer(nullptr, static_cast<UINT_PTR>(m_timerId));
    activeTimers().erase(static_cast<UINT_PTR>(m_timerId));
    m_timerId = 0;
}

void SPPlayerCanvasElement::setImplementation(SPPlayerCanvasImplementationElement* impl) {
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

    notify("implementationChanged", this, nullptr);
}

void SPPlayerCanvasElement::setAudioFps(int fps) {
    if (fps <= 0) return;
    m_audioFps = fps;
    if (m_timerId) {
        stopTimer();
        startTimer();
    }
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

int SPPlayerCanvasElement::onPlaybackChanged(SPObject*, SPEventArgs* args) {
    auto* pba = dynamic_cast<SPPlaybackStateEventArgs*>(args);
    if (!pba) return 0;

    if (pba->playing) {
        startTimer();
    } else {
        stopTimer();
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
