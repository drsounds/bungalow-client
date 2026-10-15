#pragma once

#include "SPVBoxElement.h"
#include "SPPlayerCanvasImplementationElement.h"

namespace space {
namespace views {

/// Runtime slot for SPPlayerCanvasImplementationElement (Win32 build).
/// Forwards playback events to the active implementation and drives
/// onAudioFrame() from a Win32 timer while playing.
class SPPlayerCanvasElement : public SPVBoxElement {
public:
    explicit SPPlayerCanvasElement(SPElement* parent);
    ~SPPlayerCanvasElement() override;

    std::string getType() override { return "playercanvas"; }

    void setImplementation(SPPlayerCanvasImplementationElement* impl);
    SPPlayerCanvasImplementationElement* getImplementation() const { return m_current; }

    void setAudioFps(int fps);
    int  getAudioFps() const { return m_audioFps; }

    void onAudioTimerFired();

private:
    SPPlayerCanvasImplementationElement* m_current  = nullptr;
    unsigned long long                   m_timerId  = 0;
    int                                  m_audioFps = 30;

    void startTimer();
    void stopTimer();
    int onPlaybackChanged(SPObject* sender, SPEventArgs* args);
};

} // namespace views
} // namespace space
