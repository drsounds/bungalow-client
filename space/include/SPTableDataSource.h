#ifndef TABLEVIEWDATASOURCE_H
#define TABLEVIEWDATASOURCE_H
#include <string>
#include <vector>
using namespace std;

namespace space {
class SPTableCell;
class SPTableDataSource
{
    public:

        SPTableDataSource();
        virtual ~SPTableDataSource();
        virtual int getColumnCount() { return 0; }
        virtual int getRowCount() { return 0; }
        virtual string getUriForCell (int column, int row) { return ""; }
        virtual string getTextForCell (int column, int row) { return ""; }
        virtual string getTextForColumn (int column) { return ""; }
        virtual string getRowUri(int index) { return ""; }
        virtual bool isRowActive(int index) { return false; }
        virtual bool isCellActive (int index) { return false; }
        virtual void *getDataForCell(int index) { return NULL; }
        virtual bool compareTableCells(int column, SPTableCell *t1, SPTableCell *t2) { return false; }

        /// Returns an application-defined integer identifier for the given row
        /// (e.g. a database track ID).  Returns 0 if not applicable.
        virtual int getTrackIdForRow(int /*row*/) { return 0; }

        /// Returns a progress value in [0, 1] for the given row, or -1 if no
        /// progress bar should be drawn.  When >= 0 the last column of that row
        /// renders a filled progress bar instead of text.
        virtual float getProgressForRow(int /*row*/) { return -1.f; }

        /// Returns false if the row's track cannot be played (no playable URI
        /// is associated with it).  The table uses a muted color for such rows.
        virtual bool isRowPlayable(int /*row*/) { return true; }

        virtual vector<int> getSelectedIndexes() { return vector<int>(); }
        virtual bool isIndexSelected (int index) { return false; }
        virtual void addIndexesToSelection(int index) {}
        virtual void removeIndexesFromSelection (int index) {}

    protected:
    private:
};
}
#endif // TABLEVIEWDATASOURCE_H
