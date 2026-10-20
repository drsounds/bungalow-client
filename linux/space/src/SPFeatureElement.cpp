//
// Created by Alexander Forselius on 2025-12-12.
//

#include "SPFeatureElement.h"

#include "SPViewElement.h"

namespace space {
void SPFeatureElement::registerView(SPViewElement* view) {
	this->m_views.push_back(view);
	this->appendChild(view);
}

SPFeatureElement::SPFeatureElement(SPElement *parent) : SPViewStackElement(parent) {
	this->setTagName("feature");
}

}