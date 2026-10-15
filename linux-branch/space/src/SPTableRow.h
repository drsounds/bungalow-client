#pragma once
#include <vector>
#include <string>
using namespace std;
namespace space {
class SPTableElement;
class SPTableRow
{
    public:
        bool isSelected;
        string uri;
        string title;
        SPTableRow(SPTableElement *tableView);
        SPTableElement *tableElement;
        virtual ~SPTableRow();
        std::vector<SPTableRow *> children;
};
}

