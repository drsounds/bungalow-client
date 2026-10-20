#include "SPTreeItem.h"
#include "SPUri.h"
#include <regex>
namespace space {
    SPTreeItem::SPTreeItem() : uri(nullptr) {
        this->setText("");
    }
    SPTreeItem::SPTreeItem(SPUri *uri) {
        this->uri = uri;
        this->setText("Loading...");
        if (uri && std::regex_match(uri->getAddress(), std::regex("bungalow:internal:start"))) {
            this->setText("Start");
        }
    }
}
