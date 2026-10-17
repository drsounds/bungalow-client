#include "SPResourceElement.h"
#include "SPGraphicsContext.h"
#include "SPFontStyle.h"
#include "SPResource.h"
#include <regex>

namespace space {
    SPResourceElement::SPResourceElement(SPResource *resource, SPElement *parent)
     : SPElement(parent)
    {
    	this->setTagName("resource");
        //ctor
        this->resource = resource;
    }
    void SPResourceElement::paint(int x, int y, SPGraphicsContext *g) {
        SPFontStyle font = this->getFontStyle();
        SPColor fgColor = this->getColorAttribute("fgcolor", SPColor(255, 255, 255, 255));
        SPColor bgColor = this->getColorAttribute("bgcolor", SPColor(0, 0, 0, 255));
        if (this->resource != NULL) {
            const int absX = x + this->getX();
            const int absY = y + this->getY();
            this->absoluteBounds = {
                absX, absY, this->getWidth(), this->getHeight()
            };

            // draw title
            string name = this->resource->name;
            string title = name;
            sp_rectangle txtSize = g->measureString(title, font);
            int top = absY + (this->getHeight() / 2) - (txtSize.height / 2);
            g->drawString(title, font, fgColor, absX + 10, top, this->getWidth(), this->getHeight());

            // draw separator
            const int bottom = absY + this->getHeight() - 1;
            g->drawLine(absX, bottom, absX + this->getWidth(), bottom, fgColor);
        }
    }
    SPResourceElement::~SPResourceElement()
    {
        //dtor
    }
}
