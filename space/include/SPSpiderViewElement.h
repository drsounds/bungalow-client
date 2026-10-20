//
// Created by Alexander Forselius on 2025-12-14.
//

#ifndef BUNGALOWCLIENT_SPSPIDERVIEW_H
#define BUNGALOWCLIENT_SPSPIDERVIEW_H

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


#endif //BUNGALOWCLIENT_SPSPIDERVIEW_H