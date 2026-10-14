#ifndef TABBARVIEWELEMENT_H
#define TABBARVIEWELEMENT_H

#include "SPViewElement.h"

namespace space {
class SPViewElement;
class SPViewStackElement;
class SPTabBarElement;
class SPTabStackElement;
class SPTabBarViewElement : public SPViewElement {
public:
	string getType() override { return "tabbarview"; }
	bool acceptsUri(const string &uri) override;
	void onLoad(SPObject *sender, SPEventArgs *e);
	virtual void setHash(const string &hash) override;
	int onTabSelected(SPObject *sender, SPEventArgs *e);
	SPTabBarViewElement();
	SPTabBarViewElement(SPElement *parent);
	SPElement *getViewStackElement() override;
	void onClick(SPObject *sender, SPEventArgs *e);
	void clear() override;
	virtual SPElement *getSlotElement() override;
	void onUnload(SPObject *sender, SPEventArgs *e);
	void onTreeChanged(SPObject *sender, SPEventArgs *e);
	virtual ~SPTabBarViewElement();
	virtual void navigate(const string &uri) override;
	void mouseDown(SPObject *sender, SPEventArgs *e) override;

protected:
	SPTabStackElement *tabStack;
	SPTabBarElement *tabBar;

private:
};
} // namespace space

#endif // TABBARVIEWELEMENT_H
