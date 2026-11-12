#include "SPImageElement.h"

#include "SPGraphicsContext.h"

namespace space {
SPImageElement::SPImageElement(SPElement *parent)
 : SPElement(parent)
{
	this->setTagName("img");
    //ctor
}
void SPImageElement::paint(int x, int y, SPGraphicsContext *g) {
    g->drawImage(m_image, x, y, this->getWidth(), this->getHeight());
}
SPImageElement::~SPImageElement()
{
    //dtor
}
}
