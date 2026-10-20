//
// Created by Alexander Forselius on 2025-12-18.
//

#pragma once

#include "SPElement.h"

namespace space {

class SPTabStackElement : public SPElement
{
private:
	string m_tabId;
public:
	string getType() override {
		return "tabstack";
	}
	string getTabId() {
		return m_tabId;
	}
	void setTabId(const string& tabId);
	explicit SPTabStackElement(SPElement *parent);
	void pack() override;
};
}