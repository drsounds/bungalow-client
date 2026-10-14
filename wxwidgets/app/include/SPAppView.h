#pragma once

#include "SiteView.h"
#include "SPPlayerCanvasImplementationElement.h"

namespace space {
namespace views {

/// Web-view canvas implementation for SPPlayerCanvasElement.
///
/// Presents a full SiteView (wxWebView) with no navigation bar.
/// When active:
///   - Audio data (PCM + FFT) is pushed each frame via RunScript into a
///     window.spacifyAudioFrame({pcm:[...], fft:[...]}) callback.
///   - Playback state changes are pushed via window.spacifyPlaybackChanged({...}).
///
/// Web pages hosted here register these callbacks to drive their own
/// visualizations or lyrics/metadata displays.
///
/// URI prefix: "spacify:player:app:" by default.  Subclasses may override
/// uriToUrl() to translate the URI into a page URL.
class SPAppView
    : public SiteView
    , public SPPlayerCanvasImplementationElement
{
public:
    explicit SPAppView(SPElement* parent,
                       const std::string& uriPrefix = "spacify:player:app:");

    // ── SPElement / SiteView ──────────────────────────────────────────────────
    std::string getType() override { return "appview"; }

    // ── SPPlayerCanvasImplementationElement ───────────────────────────────────
    std::string getName()    const override { return "App View"; }
    SPElement*  asElement()        override { return this; }

    void onActivated()   override;
    void onDeactivated() override;

    void onPlaybackChanged(bool playing, int posMs, int durMs) override;

    /// Injects the latest PCM and FFT frame as JSON into the hosted web page.
    /// The page should define window.spacifyAudioFrame(data) to receive it.
    ///
    /// Data is capped at 256 PCM samples and 256 FFT bins per frame to keep
    /// the serialised payload small.
    void onAudioFrame(const float* pcm, int pcmCount,
                      const float* fft, int fftCount) override;
};

} // namespace views
} // namespace space
