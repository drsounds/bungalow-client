#include "SPBoxElement.h"

#include <iostream>

#include "SPElement.h"
#include "SPWindowElement.h"
#include "SPNode.h"

namespace space {
SPBoxElement::SPBoxElement()
 : SPElement()
{
    //ctor
}

SPBoxElement::SPBoxElement(SPElement *parent)
 : SPElement(parent)
{
	this->setTagName("box");

}

SPBoxElement::~SPBoxElement()
{
    //dtor
}
/**
 Box Element
**/
void SPBoxElement::pack() {
	SPElement::pack();
	// cout << "Element: " << this->getType() << "#" << this->getId() << " packed to width: " << this->getWidth() << " height: " << this->getHeight() << " flex: " << this->getFlex() << endl;
	vector<SPNode *> nodes = this->getChildNodes();
    if (nodes.size() < 1) {
        return;
    }
	SPNode *elm = (SPNode *)static_cast<space::SPNode *>(*nodes.begin());
	SPElement *child = (SPElement *)elm;
	child->setX(0);
	child->setY(0);
	child->setWidth(this->getWidth());
	child->setHeight(this->getHeight());
	child->pack();


};
}
