#include "SPViewElement.h"
#include <string>
#include <iostream>
using namespace std;

namespace space {
SPViewElement::SPViewElement() {
	this->setTagName("view");
}
SPViewElement::SPViewElement(SPElement *parent)
 : SPVBoxElement(parent)
{
	this->setTagName("view");
    this->clipView = true;
    //ctor
}
void SPViewElement::scroll(int scrollX, int scrollY, int mouseX, int mouseY) {
    if (this->clipView) {
        this->scrollX += x;
        this->scrollY += y;
        this->invalidate();
    }
}
void SPViewElement::pack() {
	SPVBoxElement::pack();
	for (auto child : this->getChildNodes()) {
		SPElement *elm = dynamic_cast<SPElement *>(child);
		elm->pack();
	}

}
void SPViewElement::navigate(const string& uri) {

}

bool SPViewElement::acceptsUri(const string &uri) {
	return false;
}

SPViewElement::~SPViewElement()
{
    //dtor
}
}
