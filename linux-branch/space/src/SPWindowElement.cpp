#include "SPWindowElement.h"

#include <iostream>

#include "SPGraphicsContext.h"

#include "SPSkin.h"
#include "SPElement.h"
#include "../../spacify/src/MainElement.h"
#include "Space.h"
#include "SPElement.h"
#include "SPNavigateEventArgs.h"

namespace space
{
    class Space;

    SPWindowElement::SPWindowElement(Space *space, SPSkin *skin) : SPElement(nullptr)
    {
    	this->setTagName("window");
        this->setSpace(space);
        this->setSkin(skin);
    }

    void SPWindowElement::draw(SPGraphicsContext *gc, int x, int y) {
        std::vector<SPNode *> elements = this->getChildNodes();

        for (auto node : elements) {
            auto *elm = dynamic_cast<SPElement *>(node);
            if (elm->isVisible())
            {
                elm->paint(0, 0, gc);
            }
        }
    }
    void SPWindowElement::invalidateRegion(sp_rectangle region)
    {
        auto *gc = this->createGraphics();
        
        if (gc == nullptr) {
            cerr << "GraphicsContext is NULL" << endl;
            return;
        }

        gc->invalidateRegion(region);
    }
    void SPWindowElement::navigate(const string& uri) {
        SPNavigateEventArgs *args = new SPNavigateEventArgs(uri);
        this->dispatchEvent("navigate", args, this);
    }
    void SPWindowElement::pack() {

        auto *gc = this->createGraphics();

        int width = gc->getWidth();
        int height = gc->getHeight();
        for (auto & it : this->getChildNodes()) {
            auto *elm = dynamic_cast<SPElement *>(it);
            elm->setX(0);
            elm->setY(0);
            elm->setWidth(width);
            elm->setHeight(height);
            elm->pack();
        }
    }
}
