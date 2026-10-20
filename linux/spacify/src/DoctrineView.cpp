#include "./DoctrineView.h"

#include <regex>
#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
#include "../../space/src/SPTextElement.h"
#include "../../space/src/SPElement.h"
#include "../../space/src/SPGraphicsContext.h"

using namespace std;

namespace space {
    namespace views {
        DoctrineView::DoctrineView(SPElement *parent)
         : SPContainerViewElement(parent)
        {
            this->setPadding({ 20, 20, 20, 20 });
            //ctor
            text = new SPTextElement(this);
            text->setStringAttribute("width", "100%");
            this->appendChild(text);
        }

        bool DoctrineView::acceptsUri(const string& uri) {
            string uri2 = uri.c_str();
            bool a = std::regex_match(uri2, regex("bungalow:doctrine:(.*)"));
            return a;
        }

        void DoctrineView::navigate(const string& uri) {

            vector<string> strs;
            boost::split(strs, uri, boost::is_any_of(":")); 

        }

        DoctrineView::~DoctrineView()
        {
            //dtor
        }
    }
}
