#include "WhatsNewView.h"

#include "../../space/src/SPButtonElement.h"
#include "../../space/src/SPHBoxElement.h"
#include "../../space/src/SPTableElement.h"

#include <regex>

namespace space {
    namespace views {
        WhatsNewView::WhatsNewView()
         : SPContainerViewElement::SPContainerViewElement()
        {
            this->clipView = true;
            //ctor
        }

        bool WhatsNewView::acceptsUri(const string& uri) {
            bool ret = std::regex_match(uri.c_str(), std::regex("bungalow:whatsnew"));
        	return ret;
        }
        WhatsNewView::WhatsNewView(SPElement *parent)
         : SPContainerViewElement::SPContainerViewElement(parent)
        {
            // Build the view
        	this->setAttribute("src", "bungalow:whatsnew");
        }
        void WhatsNewView::navigate(const string& uri) {

        }
        WhatsNewView::~WhatsNewView()
        {
            //dtor
        }
    }
}
