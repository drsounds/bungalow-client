#include "SpaceView.h"
#include "../../space/src/SPSpiderViewElement.h"

#include <iostream>
#include <regex>

namespace space {
    namespace views {
        SpaceView::SpaceView() {
        }

        SpaceView::SpaceView(SPElement *parent)
            : SPSpiderViewElement(parent)
        {
            this->clipView = false;
        }

        string SpaceView::getType() {
            return "spaceview";
        }
        bool SpaceView::acceptsUri(const string& value) {
            return true; 
        }
        void SpaceView::navigate(const string& value) {
            this->setUri(value);
          	this->setAttribute("src", value);
        }

        SpaceView::~SpaceView()
        {
        }
    }
}
