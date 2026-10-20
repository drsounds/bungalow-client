/// SPAppView.cpp
/// Web-view canvas implementation for SPPlayerCanvasElement.

#include "SPAppView.h"

#include <algorithm>
#include <string>

namespace space {
namespace views {

SPAppView::SPAppView(SPElement* parent, const std::string& uriPrefix)
    : SiteView(parent,
               uriPrefix,
               /*showNavBar=*/     false,
               /*showUrlBar=*/     false,
               /*showBookmark=*/   false)
{
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void SPAppView::onActivated() {
    setVisible(true);
}

void SPAppView::onDeactivated() {
    setVisible(false);
}

// ── Playback events ───────────────────────────────────────────────────────────

void SPAppView::onPlaybackChanged(bool playing, int posMs, int durMs) {
    std::string js =
        "if(typeof window.bungalowPlaybackChanged==='function'){"
            "window.bungalowPlaybackChanged({"
                "playing:"    + std::string(playing ? "true" : "false") +
                ",positionMs:" + std::to_string(posMs) +
                ",durationMs:" + std::to_string(durMs) +
            "});"
        "}";
    runScript(js);
}

// ── Audio data injection ──────────────────────────────────────────────────────

void SPAppView::onAudioFrame(const float* pcm, int pcmCount,
                              const float* fft, int fftCount) {
    // Cap array sizes to keep the JSON payload manageable.
    const int maxPcm = std::min(pcmCount, 256);
    const int maxFft = std::min(fftCount, 256);

    // Build compact JSON: {"pcm":[...],"fft":[...]}
    // Using snprintf for each float is faster than std::to_string and avoids
    // locale-dependent decimal separators.
    std::string js;
    js.reserve(64 + maxPcm * 10 + maxFft * 10);

    js = "if(typeof window.bungalowAudioFrame==='function'){"
             "window.bungalowAudioFrame({pcm:[";

    char buf[32];
    for (int i = 0; i < maxPcm; ++i) {
        if (i > 0) js += ',';
        snprintf(buf, sizeof(buf), "%.4f", pcm[i]);
        js += buf;
    }

    js += "],fft:[";

    for (int i = 0; i < maxFft; ++i) {
        if (i > 0) js += ',';
        snprintf(buf, sizeof(buf), "%.4f", fft[i]);
        js += buf;
    }

    js += "]});}";

    runScript(js);
}

} // namespace views
} // namespace space
