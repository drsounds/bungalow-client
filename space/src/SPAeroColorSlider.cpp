#include "SPAeroColorSlider.h"
#include "SPGraphicsContext.h"
#include "SPMouseEventArgs.h"
#include "SPEventArgs.h"
#include "libspace.h"
#include <cmath>

namespace space {

// ── Simple event args carrying the new slider value ───────────────────────────

class SPAeroSliderEventArgs : public SPEventArgs {
public:
    explicit SPAeroSliderEventArgs(float v) : value(v) {}
    float value;
};

// ── Constructor ───────────────────────────────────────────────────────────────

SPAeroColorSlider::SPAeroColorSlider(SPElement* parent, Type type)
    : SPElement(parent), m_type(type)
{
    // Sensible defaults per type
    switch (type) {
        case Type::Hue:        m_value = 200.0f; break;
        case Type::Saturation: m_value = 0.7f;   break;
        case Type::Lightness:  m_value = 0.5f;   break;
    }
    setHeight(kKnobR * 2 + 2);

    this->addEventListener(string("mousedown"), SPCallback(&SPAeroColorSlider::mouseDown), this, this);
    this->addEventListener(string("mouseup"),   SPCallback(&SPAeroColorSlider::mouseUp),   this, this);
    this->addEventListener(string("mousemove"), SPCallback(&SPAeroColorSlider::onMouseMove), this, this);
}

// ── Value setter ──────────────────────────────────────────────────────────────

void SPAeroColorSlider::setValue(float v)
{
    if (m_type == Type::Hue) {
        while (v < 0.0f)    v += 360.0f;
        while (v >= 360.0f) v -= 360.0f;
    } else {
        if (v < 0.0f) v = 0.0f;
        if (v > 1.0f) v = 1.0f;
    }
    m_value = v;
    invalidate();
}

// ── Rendering ─────────────────────────────────────────────────────────────────

void SPAeroColorSlider::paint(int px, int py, SPGraphicsContext* g)
{
    const int absX = px + getX();
    const int absY = py + getY();
    const int w    = getWidth();
    const int h    = getHeight();
    absoluteBounds = { absX, absY, w, h };

    const int trackY = absY + h / 2 - kTrackH / 2;
    const int trackX = absX + kPadding;
    const int trackW = w - kPadding * 2;

    // ── Draw gradient track ───────────────────────────────────────────────────
    if (m_type == Type::Hue) {
        // Six-segment rainbow: 0→60→120→180→240→300→360
        const float hStops[] = { 0.f, 60.f, 120.f, 180.f, 240.f, 300.f, 360.f };
        int nSegs = 6;
        int segW = trackW / nSegs;
        for (int i = 0; i < nSegs; ++i) {
            SPColor startC = SPColor::fromHSL(hStops[i],   1.0f, 0.55f);
            SPColor endC   = SPColor::fromHSL(hStops[i+1], 1.0f, 0.55f);
            int sx = trackX + i * segW;
            int sw = (i == nSegs - 1) ? (trackW - i * segW) : segW;
            SPGradient grad(startC, endC, 0);
            g->fillRoundedLinearGradientRectangle(sx, trackY, sw, kTrackH, 0, 0, grad);
        }
    } else if (m_type == Type::Saturation) {
        SPColor grayC = SPColor::fromHSL(m_linkedHue, 0.0f, 0.55f);
        SPColor satC  = SPColor::fromHSL(m_linkedHue, 1.0f, 0.55f);
        SPGradient grad(grayC, satC, 0);
        g->fillRoundedLinearGradientRectangle(trackX, trackY, trackW, kTrackH, 0, 0, grad);
    } else { // Lightness
        SPColor black = SPColor::fromHSL(m_linkedHue, m_linkedSat, 0.05f);
        SPColor mid   = SPColor::fromHSL(m_linkedHue, m_linkedSat, 0.55f);
        SPColor white = SPColor::fromHSL(m_linkedHue, m_linkedSat * 0.3f, 0.95f);
        int half = trackW / 2;
        SPGradient g1(black, mid,   0);
        SPGradient g2(mid,   white, 0);
        g->fillRoundedLinearGradientRectangle(trackX,        trackY, half,          kTrackH, 0, 0, g1);
        g->fillRoundedLinearGradientRectangle(trackX + half, trackY, trackW - half, kTrackH, 0, 0, g2);
    }

    // ── Track border ──────────────────────────────────────────────────────────
    g->drawRoundedRectangle(trackX, trackY, trackW, kTrackH, 4,
                            SPColor(0, 0, 0, 60));

    // ── Knob ──────────────────────────────────────────────────────────────────
    float ratio = (m_type == Type::Hue) ? (m_value / 360.0f) : m_value;
    const int knobCX = trackX + static_cast<int>(ratio * trackW);
    const int knobCY = absY + h / 2;

    // Shadow
    g->fillRoundedRectangle(knobCX - kKnobR + 1, knobCY - kKnobR + 1,
                            kKnobR * 2, kKnobR * 2, kKnobR, SPColor(0, 0, 0, 60));
    // White fill
    g->fillRoundedRectangle(knobCX - kKnobR, knobCY - kKnobR,
                            kKnobR * 2, kKnobR * 2, kKnobR, SPColor(255, 255, 255, 245));
    // Colour preview ring — shows the colour at current value
    SPColor preview;
    if (m_type == Type::Hue)
        preview = SPColor::fromHSL(m_value, 0.85f, 0.55f);
    else if (m_type == Type::Saturation)
        preview = SPColor::fromHSL(m_linkedHue, m_value, 0.55f);
    else
        preview = SPColor::fromHSL(m_linkedHue, m_linkedSat, m_value);
    g->drawRoundedRectangle(knobCX - kKnobR, knobCY - kKnobR,
                            kKnobR * 2, kKnobR * 2, kKnobR, preview);
    // Inner dark border
    g->drawRoundedRectangle(knobCX - kKnobR + 1, knobCY - kKnobR + 1,
                            kKnobR * 2 - 2, kKnobR * 2 - 2, kKnobR - 1,
                            SPColor(80, 80, 80, 120));
}

// ── Mouse ─────────────────────────────────────────────────────────────────────

void SPAeroColorSlider::applyMouseX(int absMouseX)
{
    const int trackX = absoluteBounds.x + kPadding;
    const int trackW = absoluteBounds.width - kPadding * 2;
    if (trackW <= 0) return;

    float ratio = static_cast<float>(absMouseX - trackX) / static_cast<float>(trackW);
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    float newVal = (m_type == Type::Hue) ? (ratio * 360.0f) : ratio;
    setValue(newVal);
    if (onChange) onChange(m_value);
    SPAeroSliderEventArgs args(m_value);
    dispatchEvent("change", &args, this);
}

void SPAeroColorSlider::mouseDown(SPObject* /*sender*/, SPEventArgs* e)
{
    auto* me = dynamic_cast<SPMouseEventArgs*>(e);
    if (!me) return;
    m_dragging = true;
    SPElement::setMouseCapture(this);
    applyMouseX(me->getX());
}

void SPAeroColorSlider::mouseUp(SPObject* /*sender*/, SPEventArgs* e)
{
    auto* me = dynamic_cast<SPMouseEventArgs*>(e);
    bool wasDragging = m_dragging;
    m_dragging = false;
    SPElement::releaseMouseCapture();
    if (wasDragging && me)
        applyMouseX(me->getX());
    if (wasDragging && onRelease) onRelease();
}

void SPAeroColorSlider::onMouseMove(SPObject* /*sender*/, SPEventArgs* e)
{
    if (!m_dragging) return;
    auto* me = dynamic_cast<SPMouseEventArgs*>(e);
    if (me) applyMouseX(me->getX());
}

} // namespace space
