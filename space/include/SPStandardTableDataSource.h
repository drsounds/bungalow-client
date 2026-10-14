#ifndef STANDARDTABLEDATASOURCE_H
#define STANDARDTABLEDATASOURCE_H
#include <vector>
#include "SPTableDataSource.h"

using namespace std;
namespace space {
    class SPTableRow;
    class SPTableColumnHeader;
class SPStandardTableDataSource : public SPTableDataSource
{
    public:
        SPStandardTableDataSource();
        virtual ~SPStandardTableDataSource();
        virtual int getColumnCount();
        virtual int getRowCount();
        virtual string getUriForCell (int column, int row);
        virtual string getTextForCell (int column, int row);
        virtual string getTextForColumn (int column);
        virtual string getRowUri(int index);
        virtual bool isRowActive(int index);
        virtual bool isCellActive (int index);
        virtual void *getDataForCell(int index);
        virtual bool compareTableCells(int column, SPTableCell *t1, SPTableCell *t2);
        vector<SPTableRow *> rows;
        vector<SPTableColumnHeader *> columnHeaders;
    protected:
    private:
};
}

#endif // STANDARDTABLEDATASOURCE_H
