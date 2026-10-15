#include "SPLinkElement.h"
namespace space {
SPLinkElement::SPLinkElement(SPElement *parent)
 : SPTextElement(parent)
{
    //ctor
	this->setTagName("link");
}

SPLinkElement::~SPLinkElement()
{
    //dtor
}
}
