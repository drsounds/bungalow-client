#include "SPAppHeaderElement.h"

#include "SPColor.h"
#include "SPElement.h"
#include "SPHBoxElement.h"
#include "SPSkin.h"

namespace space {
SPAppHeaderElement::SPAppHeaderElement(SPElement *parent) :
	SPHBoxElement(parent) {
	this->setTagName("appheader");
	// ctor
}

SPAppHeaderElement::~SPAppHeaderElement() {
	// dtor
}

} // namespace space
