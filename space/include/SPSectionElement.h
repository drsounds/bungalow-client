//
// Created by Alexander Forselius on 2025-12-18.
//

#ifndef SPACIFYCLIENT_SPSECTIONELEMENT_H
#define SPACIFYCLIENT_SPSECTIONELEMENT_H

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
#endif //SPACIFYCLIENT_SPSECTIONELEMENT_H