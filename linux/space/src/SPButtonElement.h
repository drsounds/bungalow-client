#pragma once
#include "SPElement.h"

string RGBToHex(int rNum, int gNum, int bNum);

namespace space {
	class SPColor;
	class SPGraphicsContext;
	class SPButtonElement : public SPElement {
	public:
        SPButtonElement(SPElement *parent);
		SPButtonElement();
        void pack() override;
		virtual void paint(int x, int y, SPGraphicsContext *c) override;
		string getType() override {
            return "button";
		}
	};
}
