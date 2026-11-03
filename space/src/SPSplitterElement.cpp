#include "SPSplitterElement.h"

#include "SPElement.h"
#include "SPColor.h"
#include "SPGraphicsContext.h"
#include "SPSkin.h"
#include "SPMouseEventArgs.h"

namespace space {
SPSplitterElement::SPSplitterElement(SPElement *parent)
 : SPElement(parent)
{
	this->setTagName("splitter");
    //ctor
    this->setWidth(4);

    this->addEventListener(string("mousedown"), SPCallback(&SPSplitterElement::mouseDown), this, this);
    this->addEventListener(string("mouseup"),   SPCallback(&SPSplitterElement::mouseUp),   this, this);
    this->addEventListener(string("mousemove"), SPCallback(&SPSplitterElement::onMouseMove), this, this);
}
void SPSplitterElement::paint(int x, int y, int width, int height, SPGraphicsContext *c) {
	this->getSkin()->drawElement(c, this, "", this->absoluteBounds);
}

void SPSplitterElement::mouseDown(SPObject* /*sender*/, SPEventArgs* e) {
    if (!m_target) return;
    auto* me = dynamic_cast<SPMouseEventArgs*>(e);
    if (!me) return;
    m_dragging   = true;
    m_dragStartX = me->getX();
    m_startWidth = m_target->getWidth();
    SPElement::setMouseCapture(this);
}

void SPSplitterElement::mouseUp(SPObject* /*sender*/, SPEventArgs* /*e*/) {
    m_dragging = false;
    SPElement::releaseMouseCapture();
}

void SPSplitterElement::onMouseMove(SPObject* /*sender*/, SPEventArgs* e) {
    // This fires whenever the pointer is over the splitter (or dragging it),
    // so hint a horizontal resize cursor for the platform layer to pick up.
    SPElement::setCursorHint(SPCursorHint::ResizeHorizontal);

    if (!m_dragging || !m_target) return;
    auto* me = dynamic_cast<SPMouseEventArgs*>(e);
    if (!me) return;

    int newWidth = m_startWidth + (me->getX() - m_dragStartX);
    if (newWidth < m_minWidth) newWidth = m_minWidth;
    if (newWidth > m_maxWidth) newWidth = m_maxWidth;

    m_target->setWidth(newWidth);
    if (auto* parent = m_target->getParentElement()) {
        parent->pack();
        parent->invalidate();
    }
    this->invalidate();
}

SPSplitterElement::~SPSplitterElement()
{
    //dtor
}
}
