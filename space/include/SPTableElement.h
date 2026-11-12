#ifndef TABLEVIEWELEMENT_H
#define TABLEVIEWELEMENT_H
#include "SPElement.h"
#include "SPDragDrop.h"
#include "SPTableCell.h"
#include <chrono>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;

namespace space {
class SPTableDataSource;
class SPTableRow;
class SPTableColumnHeader;
/**
 * A table of rows from a data source.
 *
 * Drag and drop: pressing a row and moving the pointer drags the selected
 * rows, as the URIs the data source gives for them (getDragUris). The table
 * is also a drop target once setDropHandler() says which drags it takes; it
 * then shows the insertion line and hands the drop over with its row index.
 *
 * Links: a cell the data source gives markup for (getMarkupForCell) is drawn
 * as rich text. Its links show a hand cursor and an underline under the
 * pointer, and a click on one navigates to it instead of selecting the row.
 */
class SPTableElement : public SPElement, public SPDropTarget
{
    public:
        /// Whether the table takes a drag.
        using DropFilter  = std::function<bool(const SPDragData&)>;
        /// Takes a drop in front of row index; returns whether it was taken.
        using DropHandler = std::function<bool(const SPDragData&, int index)>;
        void setDropHandler(DropFilter accepts, DropHandler drop) {
            m_dropFilter  = std::move(accepts);
            m_dropHandler = std::move(drop);
        }

        SPTableDataSource *dataSource;
        /** When true, zebra stripes continue past the last data row to fill
         *  the parent view's remaining height (iTunes-style empty-area fill). */
        bool fullView = false;
        bool gradientView = false;

        bool isRowSelected(int row) const { return m_selectedRows.count(row) > 0; }
        vector<int> getSelectedRows() const {
            return vector<int>(m_selectedRows.begin(), m_selectedRows.end());
        }
        int rowAtY(int absoluteY) const;
        int dropIndexAtY(int absoluteY) const;
        void setDropIndex(int index) {
            if (m_dropIndex == index) return;
            m_dropIndex = index;
            invalidate();
        }

        SPTableElement();
        SPTableElement(SPElement *parent, SPTableDataSource *dataSource);
        vector<SPTableRow *> rows;
        vector<SPTableColumnHeader *> columns;
        virtual ~SPTableElement();
        virtual string getType() override;
        virtual void paint(int x, int y, SPGraphicsContext *g) override;
        virtual void scroll(int scrollX, int scrollY, int x, int y) override;
        virtual void mouseDown(SPObject* sender, SPEventArgs* e) override;
        void mouseMove(SPObject* sender, SPEventArgs* e);
        void mouseUp(SPObject* sender, SPEventArgs* e);
        virtual void processEvent(const SPEvent& event);

        // SPDropTarget
        bool dragOver(int x, int y, const SPDragData& data) override;
        void dragLeave() override;
        bool drop(int x, int y, const SPDragData& data) override;

        /// The link under a point in window coordinates, or "".
        string linkAt(int x, int y) const;

    protected:
        void clearLinkHover() override;

    private:
        // The cells drawn as rich text in the last paint, by (row, column).
        using CellKey = std::pair<int, int>;
        std::map<CellKey, SPTableCell> m_cells;
        const SPTableCell *cellAt(int x, int y, CellKey *key = nullptr) const;
        int m_paintX = 0;        // window x of the first column (set during paint)
        int m_columnWidth = 0;   // (set during paint)
        CellKey m_hoverCell { -1, -1 };  // the cell whose link is under the pointer
        string m_pressedLink;    // the link the left button went down on
        CellKey m_pressedCell { -1, -1 };

        std::set<int> m_selectedRows;
        int m_anchorRow  = -1; // last directly-clicked or arrow-moved row
        int m_firstRowY  = 0;  // screen y of first data row (set during paint)
        int m_dropIndex = -1;
        static const int kRowH = 18;

        // A left press on a selected row, which becomes a drag once the
        // pointer has moved far enough.
        bool m_pressed = false;
        int  m_pressX  = 0;
        int  m_pressY  = 0;
        static constexpr int kDragThreshold = 4;
        void startDrag();

        DropFilter  m_dropFilter;
        DropHandler m_dropHandler;

        // Double-click detection
        using ClockType = std::chrono::steady_clock;
        ClockType::time_point m_lastClickTime {};
        int m_lastClickRow = -1;
        static constexpr int kDblClickMs = 300;
};
}
#endif // TABLEVIEWELEMENT_H
