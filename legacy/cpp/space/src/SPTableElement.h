#pragma once
#include "SPElement.h"
#include <chrono>
#include <set>
#include <string>
#include <vector>

using namespace std;

namespace space {
class SPTableDataSource;
class SPTableRow;
class SPTableColumnHeader;
class SPTableElement : public SPElement
{
    public:
        SPTableDataSource *dataSource;
        /** When true, zebra stripes continue past the last data row to fill
         *  the parent view's remaining height (iTunes-style empty-area fill). */
        bool fullView = false;
        bool gradientView = false;

        bool isRowSelected(int row) const { return m_selectedRows.count(row) > 0; }

        SPTableElement();
        SPTableElement(SPElement *parent, SPTableDataSource *dataSource);
        vector<SPTableRow *> rows;
        vector<SPTableColumnHeader *> columns;
        virtual ~SPTableElement();
        virtual string getType() override;
        virtual void paint(int x, int y, SPGraphicsContext *g) override;
        virtual void scroll(int scrollX, int scrollY, int x, int y) override;
        virtual void mouseDown(SPObject* sender, SPEventArgs* e) override;
        virtual void processEvent(const SPEvent& event);

    private:
        std::set<int> m_selectedRows;
        int m_anchorRow  = -1; // last directly-clicked or arrow-moved row
        int m_firstRowY  = 0;  // screen y of first data row (set during paint)
        static const int kRowH = 18;

        // Double-click detection
        using ClockType = std::chrono::steady_clock;
        ClockType::time_point m_lastClickTime {};
        int m_lastClickRow = -1;
        static constexpr int kDblClickMs = 300;
};
}
