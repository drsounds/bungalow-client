#include "SPTableCell.h"

#include "SPTableRow.h"

namespace space {
SPTableCell::SPTableCell(SPTableRow *tableRow)
{
    this->tableRow = tableRow;
    this->tableElement = tableRow->tableElement;
    //ctor
}

SPTableCell::~SPTableCell()
{
    //dtor
}
}
