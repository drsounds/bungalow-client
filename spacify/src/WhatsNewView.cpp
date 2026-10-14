#include "WhatsNewView.h"

#include "SPButtonElement.h"
#include "SPHBoxElement.h"
#include "SPTableElement.h"

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
            bool ret = std::regex_match(uri.c_str(), std::regex("spacify:whatsnew"));
        	return ret;
        }
        WhatsNewView::WhatsNewView(SPElement *parent)
         : SPContainerViewElement::SPContainerViewElement(parent)
        {
            // Build the view
        	this->setAttribute("src", "spacify:whatsnew");
        }
        void WhatsNewView::navigate(const string& uri) {

        }
        WhatsNewView::~WhatsNewView()
        {
            //dtor
        }
    }
}
