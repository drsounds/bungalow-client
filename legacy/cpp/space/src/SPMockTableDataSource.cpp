#include "SPMockTableDataSource.h"
#include <string>
#include <sstream>
using namespace std;
template < typename T > std::string to_string( const T& n )
{
    std::ostringstream stm ;
    stm << n ;
    return stm.str() ;
}
namespace space {


vector<int> SPMockTableDataSource::getSelectedIndexes() {
    vector<int> indexes;
    for (vector<int>::iterator it = this->selectedIndexes.begin(); it != this->selectedIndexes.end(); ++it) {
        int index = *it;
        indexes.push_back(index);
    }
    return indexes;
}
bool SPMockTableDataSource::isIndexSelected (int index) {
    return false; // (std::find(this->selectedIndexes.begin(), this->selectedIndexes.end(), index) != this->selectedIndexes.end());

}
void SPMockTableDataSource::addIndexesToSelection(int indexes) {
    this->selectedIndexes.push_back(indexes);
}
void SPMockTableDataSource::removeIndexesFromSelection (int indexes) {
    //this->selectedIndexes->remove(indexes);
}

SPMockTableDataSource::SPMockTableDataSource()
{
    //ctor
}
int SPMockTableDataSource::getColumnCount() {
    return 3;
}

int SPMockTableDataSource::getRowCount() {
    return 3;
}
SPMockTableDataSource::~SPMockTableDataSource() {

}
string SPMockTableDataSource::getUriForCell (int column, int row) {
    return "bungalow:internal:debug";
}
string SPMockTableDataSource::getTextForCell (int column, int row) {
    string t ("Row ");
    t.append(to_string<int>(row));
    t.append(" ");
    t.append(" at column ");
    t.append(to_string<int>(column));
    return t;
}    //dtor
string SPMockTableDataSource::getTextForColumn (int column) {
    string t = "Column ";
    t.append(to_string<int>(column));
    return t;
}
string SPMockTableDataSource::getRowUri(int index) {
    return "bungalow:internal:debug";
}

bool SPMockTableDataSource::isRowActive(int index) {
    if (index == 1) {
        return true;
    }
    return false;
}

bool SPMockTableDataSource::isCellActive (int index) {
    return false;
}

void *SPMockTableDataSource::getDataForCell(int index) {
    return NULL;
}

bool SPMockTableDataSource::compareTableCells(int column, SPTableCell *t1, SPTableCell *t2) {
    return false;
}
}
