//
// Created by Alexander Forselius on 2025-12-14.
//

#include "SPSpiderViewElement.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/multi_index/detail/index_matcher.hpp>

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
		vector<string> parts;
		boost::algorithm::split(parts, uri, boost::algorithm::is_any_of(":"));
		string baseUri = parts[0] + ":" + parts[1];
		this->setAttribute("src", baseUri);
	}


}