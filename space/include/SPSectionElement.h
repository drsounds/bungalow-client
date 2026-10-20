//
// Created by Alexander Forselius on 2025-12-18.
//

#ifndef BUNGALOWCLIENT_SPSECTIONELEMENT_H
#define BUNGALOWCLIENT_SPSECTIONELEMENT_H

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
#endif //BUNGALOWCLIENT_SPSECTIONELEMENT_H