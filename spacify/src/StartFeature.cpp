//
// Created by Alexander Forselius on 2025-12-12.
//

#include "StartFeature.h"

#include "StartView.h"

namespace space {
	StartFeature::StartFeature(SPElement *parent) : SPFeatureElement(parent) {
		this->registerView(new views::StartView(this));
	}
}