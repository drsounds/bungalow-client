#include "SPContainerViewElement.h"
namespace space {
    SPContainerViewElement::SPContainerViewElement(SPElement *parent)
    : SPViewElement(parent)
    {
        //ctor
        this->clipView = true;
    }

    SPContainerViewElement::SPContainerViewElement()
    : SPViewElement()
    {
        //ctor
        this->clipView = true;
    }

    SPContainerViewElement::~SPContainerViewElement()
    {
        //dtor
    }

    void SPContainerViewElement::navigate(const string &uri) {
	    SPViewElement::navigate(uri);
    }

    bool SPContainerViewElement::acceptsUri(const string &uri) {
	    return SPViewElement::acceptsUri(uri);
    }
}
