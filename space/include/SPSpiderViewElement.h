//
// Created by Alexander Forselius on 2025-12-14.
//

#ifndef SPACIFYCLIENT_SPSPIDERVIEW_H
#define SPACIFYCLIENT_SPSPIDERVIEW_H

#include "SPTabBarViewElement.h"

namespace space {
class SPSpiderViewElement : public SPTabBarViewElement
{
public:
    SPSpiderViewElement();
	explicit SPSpiderViewElement(SPElement *parent);
	~SPSpiderViewElement() override;
	string getType() override {
		return "spiderview";
	}
	virtual bool acceptsUri(const string& uri) override;
	virtual void navigate(const string &uri) override;
};
}


#endif //SPACIFYCLIENT_SPSPIDERVIEW_H