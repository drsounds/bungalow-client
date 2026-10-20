#include "SPTextElement.h"

#include "SPElement.h"
#include "SPGraphicsContext.h"
#include "SPFontStyle.h"
#include "SPColor.h"

namespace space {
SPTextElement::SPTextElement(SPElement *parent)
 : SPElement(parent)
{
	this->setTagName("text");
    //ctor
}
void SPTextElement::pack() {
    SPElement::pack();
}

SPTextElement::~SPTextElement()
{
    //dtor
}
}
