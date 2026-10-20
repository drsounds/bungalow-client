#include "UserView.h"
#include <boost/algorithm/string.hpp>
#include <string>
#include <vector>
#include <regex>
#include "../../space/src/SPHeaderElement.h"
#include "../../space/src/SPTitleElement.h"

using namespace std;
namespace space {
    namespace views {

        UserView::UserView(SPElement *parent)
         : SPViewElement(parent)
        {
            this->setPadding({ 20, 20, 20, 20 });
            //ctor
            header = new space::SPHeaderElement(this);
            header->title->setTitle("Title");
            header->title->setEntity("User");
            header->set("width", "100%");
            header->set("height", "180");
            this->appendChild(header);
        }

        UserView::UserView()
        {
            //ctor
        }

        UserView::~UserView()
        {
            //dtor
        }
        void UserView::navigate(const string& uri) {
            this->header->title->setTitle("User");
        }
        bool UserView::acceptsUri(const string& uri) {
            string uri2 = (string)uri.c_str();
            bool a = std::regex_match(uri2, std::regex("bungalow:user:(.*)"));
            return a;
        }
    }
}
