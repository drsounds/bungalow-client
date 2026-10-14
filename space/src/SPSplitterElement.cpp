#include "SPSplitterElement.h"

#include "SPElement.h"
#include "SPColor.h"
#include "SPGraphicsContext.h"
#include "SPSkin.h"

namespace space {
SPSplitterElement::SPSplitterElement(SPElement *parent)
 : SPElement(parent)
{
	this->setTagName("splitter");
    //ctor
    this->setWidth(1);
}
void SPSplitterElement::paint(int x, int y, int width, int height, SPGraphicsContext *c) {
	this->getSkin()->drawElement(c, this, "", this->absoluteBounds);
}
SPSplitterElement::~SPSplitterElement()
{
    //dtor
}
}
