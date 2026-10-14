#include "SPAppHeaderElement.h"

#include "SPElement.h"
#include "SPColor.h"

namespace space {
SPAppHeaderElement::SPAppHeaderElement(SPElement *parent) : SPHBoxElement(parent)
{
	this->setTagName("appheader");
    //ctor
}

SPAppHeaderElement::~SPAppHeaderElement()
{
    //dtor
}

}
