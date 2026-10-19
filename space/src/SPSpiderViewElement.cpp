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
		return SPTabBarViewElement::acceptsUri(uri);
	}
	void SPSpiderViewElement::navigate(const string &uri) {
		// Keep the full entity URI (spacify:artist:<id>) so the view can be
		// resolved through SPUriStore; only the #tab fragment is dropped.
		string baseUri = uri.substr(0, uri.find('#'));
		this->setAttribute("src", baseUri);
	}


}