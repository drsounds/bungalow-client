#ifndef BOXELEMENT_H
#define BOXELEMENT_H
#include "SPElement.h"
#include <string>

using namespace std;

namespace space {
class SPSkin;
class SPBoxElement : public SPElement {
public:
	explicit SPBoxElement(SPElement *parent);
	SPBoxElement();
	~SPBoxElement();
	void pack() override;
	virtual string getType() override {
		return "box";
	}

};
}
#endif // BOXELEMENT_H
