#ifndef TABLECOLUMNHEADER_H
#define TABLECOLUMNHEADER_H

#include "SPTableCell.h"
#include "SPTableElement.h"

#include <string>

using namespace std;

namespace space {
class SPTableColumnHeader {
public:
    SPTableColumnHeader(SPTableElement *tableElement);
    virtual ~SPTableColumnHeader();
    string uri;
    string title;
    SPTableElement *tableElement;
};
}

#endif // TABLECOLUMNHEADER_H
