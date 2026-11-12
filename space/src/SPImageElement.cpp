#include "SPImageElement.h"

#include "SPGraphicsContext.h"
#include "SPMouseEventArgs.h"

namespace space {
SPImageElement::SPImageElement(SPElement *parent)
 : SPElement(parent)
{
	this->setTagName("img");
    //ctor
}
void SPImageElement::paint(int x, int y, SPGraphicsContext *g) {
    const int absX = x + this->getX();
    const int absY = y + this->getY();
    this->absoluteBounds = { absX, absY, this->getWidth(), this->getHeight() };
    g->drawImage(m_image, absX, absY, this->getWidth(), this->getHeight());
}
int SPImageElement::notify(const string &event, SPObject *sender, SPEventArgs *data) {
    if (event == "mousemove" || event == "click") {
        string uri = this->getStringAttribute("uri", "");
        if (uri.empty())
            uri = this->getStringAttribute("href", "");
        auto *mouse = dynamic_cast<SPMouseEventArgs *>(data);
        const sp_rectangle b = this->getAbsoluteBounds();
        if (!uri.empty() && mouse != nullptr &&
            mouse->getX() >= b.x && mouse->getX() < b.x + b.width &&
            mouse->getY() >= b.y && mouse->getY() < b.y + b.height) {
            if (event == "mousemove") {
                this->claimLinkHover(uri);
            } else if (mouse->getButton() == 0) {
                this->setState(SP_STATE_NORMAL);
                this->activateLink(uri);
                return 1;
            }
        }
    }
    return SPElement::notify(event, sender, data);
}
SPImageElement::~SPImageElement()
{
    //dtor
}
}
