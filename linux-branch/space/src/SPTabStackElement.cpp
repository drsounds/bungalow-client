//
// Created by Alexander Forselius on 2025-12-18.
//

#include "SPTabStackElement.h"

#include "SPVBoxElement.h"

namespace space {
void SPTabStackElement::setTabId(const string& tabId) {
	this->m_tabId = tabId;

	for (auto child : this->children) {
		SPElement *elm = (SPElement *)child;
		if (elm->getId() == tabId) {
			elm->setVisible(true);
		} else {
			elm->setVisible(false);
		}
	}
	this->pack();
}
SPTabStackElement::SPTabStackElement(SPElement *parent) : SPElement(parent) {
}
void SPTabStackElement::pack() {
	for (auto child : this->children) {
		SPElement *elm = (SPElement *)child;
		elm->setWidth(this->getWidth());
		elm->setHeight(this->getHeight());
		elm->pack();
	}
}
}