//
// Created by Alexander Forselius on 2025-12-18.
//

#ifndef SPACIFYCLIENT_TABSTACKELEMENT_H
#define SPACIFYCLIENT_TABSTACKELEMENT_H

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


#endif //SPACIFYCLIENT_TABSTACKELEMENT_H