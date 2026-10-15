#include "StartView.h"

#include "../../space/src/SPSpiderViewElement.h"

#include <iostream>
#include <regex>


namespace space {
    namespace views {
        StartView::StartView() {
        }
        StartView::StartView(SPElement *parent)
         : SPSpiderViewElement(parent)
        {
            this->clipView = false;
        	this->setUri("spacify:start");
        }

        string StartView::getType() {
            return "startview";
        }

        StartView::~StartView()
        {
            //dtor
        }
    }
}
