//
// Created by Alexander Forselius on 2025-12-14.
//

#include "SPSpiderViewElement.h"

#include "SPString.h"

namespace space {

	SPSpiderViewElement::SPSpiderViewElement(SPElement *element) : SPTabBarViewElement(element) {

	}
	SPSpiderViewElement::SPSpiderViewElement() {

	}
	SPSpiderViewElement::~SPSpiderViewElement() {

	}
	bool SPSpiderViewElement::acceptsUri(const string& uri) {
		return false;
	}
	void SPSpiderViewElement::navigate(const string &uri) {
		vector<string> parts = split(uri, ':');
		string baseUri = parts[0] + ":" + parts[1];
		this->setAttribute("src", baseUri);
	}


}