#ifndef MOCKTABLEDATASOURCE_H
#define MOCKTABLEDATASOURCE_H
#include "SPTableDataSource.h"
#include <vector>
namespace space {
class SPTableDataSource;
class SPMockTableDataSource : public SPTableDataSource
{
    public:
        vector<int> selectedIndexes;
        SPMockTableDataSource();
        virtual ~SPMockTableDataSource();
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
        virtual int getColumnCountForRow(int row) { return 3; }
        virtual int getRowCountForColumn(int column) { return 3; }
        virtual vector<int> getSelectedIndexes();
        virtual bool isIndexSelected (int index);
        virtual void addIndexesToSelection(int index);
        virtual void removeIndexesFromSelection (int index);
    protected:
    private:
};
#define MOCKTABLEDATASOURCE_CLASS
}
#endif // MOCKTABLEDATASOURCE_H
