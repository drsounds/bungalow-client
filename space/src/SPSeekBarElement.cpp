#include "SPSeekBarElement.h"

#include "SPGraphicsContext.h"
#include "SPMouseEventArgs.h"
#include "SPPlaybackEventArgs.h"

namespace space {

SPSeekBarElement::SPSeekBarElement(SPElement* parent) : SPElement(parent) {
    this->setTagName("seekbar");
}

// ── Rendering ─────────────────────────────────────────────────────────────────

void SPSeekBarElement::paint(int x, int y, SPGraphicsContext* g) {
    const int absX = x + getX();
    const int absY = y + getY();
    const int w    = getWidth();
    const int h    = getHeight();

    absoluteBounds = { absX, absY, w, h };

    // Track background
    const SPColor trackBg(80, 80, 80, 255);
    g->fillRoundedRectangle(absX, absY + h / 2 - 2, w, 4, 2, trackBg);

    // Filled portion (progress)
    if (m_durMs > 0 && m_posMs >= 0) {
        float ratio = static_cast<float>(m_posMs) / static_cast<float>(m_durMs);
        if (ratio > 1.0f) ratio = 1.0f;
        const int fillW = static_cast<int>(ratio * w);
        if (fillW > 0) {
            const SPColor fillColor(95, 132, 180, 255);
            g->fillRoundedRectangle(absX, absY + h / 2 - 2, fillW, 4, 2, fillColor);
        }
        // Thumb knob
        const int knobX = absX + static_cast<int>(ratio * w) - 5;
        const int knobY = absY + h / 2 - 5;
        const SPColor knobColor(200, 200, 200, 255);
        g->fillRoundedRectangle(knobX, knobY, 10, 10, 5, knobColor);
    }
}

// ── Mouse interaction ─────────────────────────────────────────────────────────

void SPSeekBarElement::mouseDown(SPObject* /*sender*/, SPEventArgs* e) {
    auto* me = dynamic_cast<SPMouseEventArgs*>(e);
    if (!me || m_durMs <= 0) return;

    const int relX = me->getX() - absoluteBounds.x;
    const int w    = getWidth();
    if (w <= 0) return;

    float ratio = static_cast<float>(relX) / static_cast<float>(w);
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    const int seekPos = static_cast<int>(ratio * m_durMs);

    auto* args = new SPSeekEventArgs(seekPos);
    this->dispatchEvent("seek", args, this);
}

} // namespace space
