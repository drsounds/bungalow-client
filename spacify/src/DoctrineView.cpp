#include "../include/DoctrineView.h"

#include <regex>
#include <string>
#include <vector>
#include "SPString.h"
#include "SPTextElement.h"
#include "SPElement.h"
#include "SPGraphicsContext.h"

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
            bool a = std::regex_match(uri2, regex("spacify:doctrine:(.*)"));
            return a;
        }

        void DoctrineView::navigate(const string& uri) {

            vector<string> strs = split(uri, ':');

        }

        DoctrineView::~DoctrineView()
        {
            //dtor
        }
    }
}
