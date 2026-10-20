#pragma once

#include "SPElement.h"

namespace space {

class SPGraphicsContext;

/// A horizontal seek-bar that renders a filled-track indicator and dispatches
/// a "seek" event (with SPSeekEventArgs) when the user clicks on it.
///
/// The owning element should listen for "seek" and forward the position to the
/// media service via BungalowApp::seekTo().
class SPSeekBarElement : public SPElement {
public:
    explicit SPSeekBarElement(SPElement* parent);

    /// Update the current playback position (milliseconds). Triggers repaint.
    void setPosition(int posMs) { m_posMs = posMs; invalidate(); }
    /// Update the total track duration (milliseconds). Triggers repaint.
    void setDuration(int durMs) { m_durMs = durMs; invalidate(); }

    int getPosition() const { return m_posMs; }
    int getDuration() const { return m_durMs; }

    void paint(int x, int y, SPGraphicsContext* g) override;
    void mouseDown(SPObject* sender, SPEventArgs* e) override;

    std::string getType() override { return "seekbar"; }

private:
    int m_posMs = 0;
    int m_durMs = 0;
};

} // namespace space
