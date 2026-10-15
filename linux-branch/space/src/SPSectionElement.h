//
// Created by Alexander Forselius on 2025-12-18.
//

#pragma once

#include "SPVBoxElement.h"

#include <string>

namespace space {
class SPSectionElement : public SPVBoxElement
{
public:
	explicit SPSectionElement(SPElement *parent);
	void pack() override;

	string getType() override {
		return "section";
	}
};
}