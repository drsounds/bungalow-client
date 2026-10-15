#pragma once
#include <string>
using namespace std;
namespace space {
    class SPTableRow;
    class SPTableElement;
class SPTableCell
{
    public:
        SPTableCell(SPTableRow *tableRow);
        virtual ~SPTableCell();
        string uri;
        string title;
        SPTableRow *tableRow;
        SPTableElement *tableElement;
    protected:
    private:
};
}

