#ifndef VBOXELEMENT_H
#define VBOXELEMENT_H
#include "SPElement.h"

#include <string>
using namespace std;
namespace space {
	class SPElement;
	class SPWindow;
	class SPSkin;
	class SPVBoxElement : public SPElement {

	public:
		explicit SPVBoxElement(SPElement *parent);
		SPVBoxElement() {}

		virtual string getType() override {
			return "vbox";
		}
		virtual void pack() override;
	};
}
#endif // VBOXELEMENT_H
