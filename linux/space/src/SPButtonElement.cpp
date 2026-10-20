#include "SPButtonElement.h"
#include "SPGraphicsContext.h"
#include "SPColor.h"
#include "SPSkin.h"
#include "SPTextElement.h"

/**
 *
 * @param parent
 */
space::SPButtonElement::SPButtonElement(SPElement *parent) : SPElement(parent) {
	 this->tagName = "button";
}


void space::SPButtonElement::pack() {
	SPElement::pack();
	this->resizeToText(true, true);

}

void space::SPButtonElement::paint(int x, int y, SPGraphicsContext *c)
{
    this->absoluteBounds = {
    	this->getX() + x, this->getY() + y, this->getWidth(), this->getHeight()
    };

	string strClass;
	switch (this->getState()) {
	case SP_STATE_ACTIVE:
		strClass = "active";
		break;
	case SP_STATE_HOVER:
		strClass = "hover";
	}

	this->getSkin()->drawElement(c, this, strClass, this->absoluteBounds);

}
