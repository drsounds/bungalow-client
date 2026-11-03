#include "SPInputElement.h"
#include "SPSkin.h"
namespace space {
SPInputElement::SPInputElement()
{
    //ctor
}

SPInputElement::~SPInputElement()
{

}
void SPInputElement::setValue(const string& value) {
    m_value = value;
    // Backends seed a newly created native control from the attribute.
    this->setStringAttributeSilently("value", value);
    m_valuePending = true;
    if (this->getSpace() != nullptr)
        this->createGraphics()->setControlAttribute(this, "value", value);
}

void SPInputElement::focus() {
    m_focusPending = true;
    this->invalidate();
}

void SPInputElement::setVisible(bool value) {
    const bool changed = value != this->isVisible();
    SPElement::setVisible(value);
    // Hidden elements are not painted, so their native control would stay up.
    if (changed && !value && this->getSpace() != nullptr)
        this->createGraphics()->setControlVisible(this, false);
}

SPInputElement::SPInputElement(SPElement *parent)
    : SPElement(parent) {
	this->tagName = "input";
}

bool SPInputElement::isInsideClip() {
    const sp_rectangle b = this->getAbsoluteBounds();
    for (SPElement *parent = this->getParentElement(); parent != nullptr; parent = parent->getParentElement()) {
        if (parent->getOverflowY() != SPOverflow::Auto) continue;
        const sp_rectangle clip = parent->getAbsoluteBounds();
        if (b.y < clip.y || b.y + b.height > clip.y + clip.height)
            return false;
    }
    return true;
}

void SPInputElement::paint(int x, int y, SPGraphicsContext *g) {
    SPElement::paint(x, y, g);
    // Native controls are not clipped, so hide one scrolled out of view.
    if (!this->isInsideClip()) {
        g->setControlVisible(this, false);
        return;
    }
    // The native control sits inside the skin's frame so its bevel stays visible.
    const sp_rectangle bounds = this->getSkin()->getContentBounds(this, this->getAbsoluteBounds());
    g->drawInputElement(this, bounds.x, bounds.y, bounds.width, bounds.height);
    if (m_valuePending) {
        m_valuePending = false;
        g->setControlAttribute(this, "value", m_value);
    }
    if (m_focusPending) {
        m_focusPending = false;
        g->focusControl(this);
    }
}
}
