#pragma once

#include "SPElement.h"
#include "SPColor.h"
#include <functional>

namespace space {

/// Horizontal gradient slider for Frutiger Aero color tuning.
/// - Hue:        rainbow spectrum track, value 0-360
/// - Saturation: gray→saturated colour track, value 0-1
/// - Lightness:  dark→mid→white track, value 0-1
///
/// Dispatches a "change" event (SPAeroSliderEventArgs) on click/drag.
class SPAeroColorSlider : public SPElement {
public:
    enum class Type { Hue, Saturation, Lightness };

    SPAeroColorSlider(SPElement* parent, Type type);

    std::string getType() override { return "aeroslider"; }

    /// Raw value: 0-360 for Hue, 0-1 for Saturation/Lightness.
    float getValue() const { return m_value; }
    void  setValue(float v);

    /// Context hue (0-360) used by Saturation/Lightness sliders for display.
    void setLinkedHue(float h) { m_linkedHue = h; invalidate(); }
    /// Context saturation (0-1) used by Lightness slider for display.
    void setLinkedSat(float s) { m_linkedSat = s; invalidate(); }

    void paint(int x, int y, SPGraphicsContext* g) override;
    void mouseDown(SPObject* sender, SPEventArgs* e) override;
    void mouseUp(SPObject* sender, SPEventArgs* e) override;

    /// Called whenever the value changes. Provided by the owner (ConfigView).
    std::function<void(float)> onChange;

    /// Called once on mouse release. Use for deferred saves.
    std::function<void()> onRelease;

private:
    Type  m_type;
    float m_value     = 0.0f;
    float m_linkedHue = 200.0f;
    float m_linkedSat = 0.7f;
    bool  m_dragging  = false;

    static constexpr int kTrackH  = 12;
    static constexpr int kKnobR   = 8;
    static constexpr int kPadding = kKnobR;

    void applyMouseX(int absMouseX);
    void onMouseMove(SPObject* sender, SPEventArgs* e);
};

} // namespace space
