//
// Created by Alexander Forselius on 2025-12-18.
//

#include "SPPageElement.h"

namespace space {
SPPageElement::SPPageElement(SPElement *parent) :
	SPVBoxElement(parent) {
	this->setTagName("page");
	this->setFlex(1);
}
void SPPageElement::pack() { SPVBoxElement::pack(); }
} // namespace space