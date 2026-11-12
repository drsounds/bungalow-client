#include "SPInfoElement.h"
#include "SPButtonElement.h"
#include "SPTextElement.h"
#include <list>
namespace space {
    SPInfoElement::SPInfoElement(SPElement *parent)
     : SPHBoxElement(parent)
    {
    	this->setTagName("info");
        this->set("height", "32");
        this->setPadding({ 0, 0, 0, 0 });

        SPTextElement *icon = new SPTextElement(this);
        icon->set("width", "128");
        this->appendChild(icon);

        this->textElement = new SPTextElement(this);
        this->textElement->set("flex", "1");
        this->appendChild(this->textElement);

        this->btnElement = new SPButtonElement(this);
        this->btnElement->set("width", "64");
        this->btnElement->setInnerText("x");
        this->btnElement->addEventListener(
            "click",
            reinterpret_cast<SPCallback>(&SPInfoElement::onDismiss),
            this, this->btnElement);
        this->appendChild(this->btnElement);
    }

    void SPInfoElement::show(SPMessageType type, string text) {
        this->setMessageType(type);
        this->textElement->setInnerText(text);
        SPElement::show();
        this->invalidate();
    }

    int SPInfoElement::onDismiss(SPObject* /*sender*/, SPEventArgs* /*args*/) {
        this->hide();
        this->invalidate();
        return 0;
    }

    void SPInfoElement::hide() {
        SPElement::hide();
    }

    void SPInfoElement::setMessageType(SPMessageType type) {
        this->type = type;
        string bgcolor = "#ffffaa";
        switch(type) {
        case Warning:
            bgcolor = "#ffffaa";
            break;
        case Info:
            bgcolor = "#00ffff";
            break;
        case Error:
            bgcolor = "#ffeeee";
            break;
        case Message:
            bgcolor = "#eeffee";
            break;
        }
        this->set("bgcolor", bgcolor);

        for (std::vector<SPNode *>::iterator it = this->children.begin(); it != this->children.end(); ++it) {
            SPNode *node = dynamic_cast<SPNode *>(*it);
            node->set("bgcolor", bgcolor);
        }
    }

    SPInfoElement::~SPInfoElement()
    {
        //dtor
    }
}
