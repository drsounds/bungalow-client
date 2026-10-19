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
    auto *gc = this->createGraphics();
    gc->setControlAttribute(this, "value", value);
}
SPInputElement::SPInputElement(SPElement *parent)
    : SPElement(parent) {
	this->tagName = "input";
}

void SPInputElement::paint(int x, int y, SPGraphicsContext *g) {
    SPElement::paint(x, y, g);
    // The native control sits inside the skin's frame so its bevel stays visible.
    const sp_rectangle bounds = this->getSkin()->getContentBounds(this, this->getAbsoluteBounds());
    g->drawInputElement(this, bounds.x, bounds.y, bounds.width, bounds.height);
}
}
