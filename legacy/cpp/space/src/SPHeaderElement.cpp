#include "SPHeaderElement.h"

#include "SPImageElement.h"
#include "SPTitleElement.h"

namespace space {
SPHeaderElement::SPHeaderElement(SPElement *parent)
 : SPHBoxElement(parent)
{
	this->setTagName("header");
    this->setPadding({ 5, 5, 5, 5 });

    // Insert image
    this->image = new SPImageElement(this);
    this->image->set("width", "128");
    this->image->set("height", "128");
    this->image->set("flex", "0");
    this->appendChild(image);

    this->title = new SPTitleElement(this);
    title->set("flex", "1");
    title->setTitle("Title");
    title->setEntity("Object");
    title->set("height", "100%");
    title->set("width", "100%");
    this->appendChild(title);


}
SPHeaderElement::SPHeaderElement()
{
    this->setPadding({ 20, 20, 20, 20 });
    this->title = new SPTitleElement(this);
    title->set("flex", "1");
    title->setTitle("Title");
    title->setEntity("Object");
    title->set("height", "100%");
    this->appendChild(title);


}

string SPHeaderElement::getType() {

    return "headerelement";
}

SPHeaderElement::~SPHeaderElement()
{
    //dtor
}
}
