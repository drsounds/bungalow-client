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
    const int absX = x + this->getX();
    const int absY = y + this->getY();
    this->absoluteBounds = { absX, absY, this->getWidth(), this->getHeight() };
    g->drawImage(m_image, absX, absY, this->getWidth(), this->getHeight());
}
SPImageElement::~SPImageElement()
{
    //dtor
}
}
