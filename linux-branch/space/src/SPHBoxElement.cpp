#include "SPHBoxElement.h"

#include <iostream>

#include "SPElement.h"


namespace space {
	SPHBoxElement::SPHBoxElement(SPElement *parent) : SPElement(parent) {
		this->setTagName("hbox");
	}
    void SPHBoxElement::pack() {
        SPElement::pack();
        int flexes = 0;
        int totalFlex = 0;
        int statics = 0;
        int leftWidth = 0;
        int rightWidth = 0;
        int side = -1;
        int gap = this->getGap();
        // First calculate count of flex
        for(std::vector<SPNode *>::iterator it = this->getChildNodes().begin(); it != this->getChildNodes().end(); ++it) {
        	SPElement *child = (SPElement *)*it;
        	if (!child->isVisible()) {
        		continue;
        	}
            if(child->getFlex() > 0) {
                flexes++;
                totalFlex += child->getFlex();
                side = 0;
            } else {
                if(side == 0)
                    side+=1;
                if(side > 1)
                    side = 1;
                statics++;
                if(side < 1)
                    leftWidth += child->getWidth();
                else if(side > 0)
                    rightWidth += child->getWidth();
            }
        }

        int availableFlexWidth = totalFlex > 0 ? (this->getWidth() - leftWidth - rightWidth) : 0;
        if (availableFlexWidth < 1)
        {
            availableFlexWidth = 1;
        }
        for(std::vector<SPNode *>::iterator it = this->getChildNodes().begin(); it != this->getChildNodes().end(); ++it) {
            SPElement *child = (SPElement *)*it;
        	if (!child->isVisible()) {
        		continue;
        	}
        	int leftPadding = this->getPadding().left + child->getMargins().left;
        	int rightPadding = this->getPadding().right + child->getMargins().right;
            int flex = child->getFlex();
            if(flex > 0) {
            	child->setWidth((flex * availableFlexWidth) / totalFlex - leftPadding - rightPadding - gap);
            } else {
                statics++;
            }
        }

        // Now assign elements
        int left = 0;
        int right = 0;
        side = -1;
        for(std::vector<SPNode *>::iterator it = this->getChildNodes().begin(); it != this->getChildNodes().end(); ++it) {
            SPElement *child = (SPElement *)*it;
        	if (!child->isVisible()) {
        		continue;
        	}
        	int leftPadding = this->getPadding().left + child->getMargins().left;
        	int topPadding = this->getPadding().top + child->getMargins().top ;
        	int bottomPadding = this->getPadding().bottom + child->getMargins().bottom;
            if(child->getFlex() > 0)
            {

                side = 0;
            }
            else
            {
                if(side == 0)
					side = 1;
            }
            
            if(side < 0) {
                int x = left + leftPadding;
                if (it != this->getChildNodes().begin()) {
                    x += gap;
                }
                child->setX(x);
            }
            else if(side == 0)
            {
                int x = leftWidth + leftPadding;
                if (it != this->getChildNodes().begin()) {
                    x += gap;
                }
                child->setX(x);
            }
            else
            {
                int x = leftPadding + leftWidth + availableFlexWidth + right;
                if (it != this->getChildNodes().begin()) {
                    x += gap;
                }
                child->setX(x);
            }
            child->setY(topPadding);
            child->setHeight(this->getHeight() - bottomPadding - topPadding);

			switch(side) {
            case -1:
                left += child->getWidth();
                break;
            case 0:
                break;
			default:
                break;
            }
        }

        // Pack child elements
        for(std::vector<SPNode *>::iterator it = this->getChildNodes().begin(); it != this->getChildNodes().end(); ++it) {
            SPElement *child = (SPElement *)*it;
            if (!child->isVisible())
                continue;
            child->pack();
        }
        // cout << "Element: " << this->getType() << " packed to width: " << this->getWidth() << " height: " << this->getHeight() << " flex: " << this->getIntegerAttribute("flex", 0) << endl;
    }
}