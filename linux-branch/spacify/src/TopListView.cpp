#include "TopListView.h"
#include "../../space/src/SPHBoxElement.h"
#include "../../space/src/SPTableElement.h"
#include "../../space/src/SPMockTableDataSource.h"
#include "../../space/src/SPVBoxElement.h"
#include <regex>
namespace space {
    namespace views {
        TopListView::TopListView() :
            SPContainerViewElement::SPContainerViewElement() {
        }
        string TopListView::getType() {
            return "toplistview";
        }
        TopListView::TopListView(SPElement *parent)
            : SPContainerViewElement::SPContainerViewElement(parent)
        {

        }
        void TopListView::scroll(int scrollX, int scrollY, int x, int y) {
            SPElement::scroll(scrollX, scrollY, x, y);
        }
        void TopListView::navigate(const string& uri) {

        }
        bool TopListView::acceptsUri(const string& uri) {
            return std::regex_match(uri.c_str(), std::regex("spacify:internal:toplist"));
        }
        TopListView::~TopListView()
        {
            //dtor
        }
    }
}
