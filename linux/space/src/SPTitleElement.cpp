#include "SPTitleElement.h"

#include "SPColor.h"

namespace space {
SPTitleElement::SPTitleElement()
{
    //ctor
}

SPTitleElement::~SPTitleElement()
{

}
void SPTitleElement::setTitle(string title) {

    this->title->setInnerText(title);
}
string SPTitleElement::getTitle() {
    return this->title->getInnerText();
}

void SPTitleElement::setEntity(string entity) {

    this->title->setInnerText(entity);
}
string SPTitleElement::getEntity() {
    return this->entity->getInnerText();
}
SPTitleElement::SPTitleElement(SPElement *parent)
 : SPVBoxElement(parent){
	this->setTagName("title");
    entity = new SPTextElement(this);
    entity->setInnerText(this->getEntity());
    entity->setIntegerAttribute("height", 180);
    entity->setColorAttribute("fgcolor", SPColor(10, 10, 10, 1));
    entity->setIntegerAttribute("size", 15);
    entity->set("width", "100%");
    this->appendChild(entity);

    title = new SPTextElement(this);
    title->setInnerText(this->getTitle());
    title->setIntegerAttribute("height", 160);
    title->setIntegerAttribute("size", 25);
    title->set("width", "100%");
    this->appendChild(title);
}
}
