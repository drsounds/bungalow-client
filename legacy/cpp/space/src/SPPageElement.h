//
// Created by Alexander Forselius on 2025-12-18.
//

#pragma once

#include "SPVBoxElement.h"

#include <string>

namespace space {
class SPPageElement : public SPVBoxElement {
public:
	explicit SPPageElement(SPElement *parent);
	void pack() override;

	string getType() override { return "page"; }
};
} // namespace space