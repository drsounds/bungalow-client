#include "SPStandardTableDataSource.h"
#include <string>
#include <sstream>
using namespace std;
namespace space {
SPStandardTableDataSource::SPStandardTableDataSource()
{

}
int SPStandardTableDataSource::getColumnCount() {
    return this->columnHeaders.size();
}

int SPStandardTableDataSource::getRowCount() {
    return this->rows.size();
}
SPStandardTableDataSource::~SPStandardTableDataSource() {
}
string SPStandardTableDataSource::getUriForCell (int column, int row) {
    return "spacify:internal:debug";
}
string SPStandardTableDataSource::getTextForCell (int column, int row) {
    string t ("Row ");
    t.append(std::to_string(row));
    t.append(" ");
    t.append(" at column ");
    t.append(std::to_string(column));
    return t;
}    //dtor
string SPStandardTableDataSource::getTextForColumn (int column) {
    string t = "Column ";
    t.append(std::to_string(column));
    return t;
}
string SPStandardTableDataSource::getRowUri(int index) {
    return "spacify:internal:debug";
}

bool SPStandardTableDataSource::isRowActive(int index) {
    if (index == 1) {
        return true;
    }
    return false;
}

bool SPStandardTableDataSource::isCellActive (int index) {
    return false;
}

void *SPStandardTableDataSource::getDataForCell(int index) {
    return NULL;
}

bool SPStandardTableDataSource::compareTableCells(int column, SPTableCell *t1, SPTableCell *t2) {
    return false;
}
}
