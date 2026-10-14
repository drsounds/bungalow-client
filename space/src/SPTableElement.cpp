#include "SPTableElement.h"

#include "SPSkin.h"
#include "SPGraphicsContext.h"
#include "SPTableDataSource.h"
#include "SPMouseEventArgs.h"
#include "SPEventArgs.h"   // SPKeyEventArgs
#include "SPPlaybackEventArgs.h"  // SPTableRowEventArgs
#include "SPApplicationContext.h"

#include <algorithm>
#include <chrono>

namespace space {
    SPTableElement::SPTableElement()
    {
        //ctor
    }
    void SPTableElement::scroll(int scrollX, int scrollY, int x, int y) {
        SPElement::scroll(scrollX, scrollY, x, y);
    }
    SPTableElement::SPTableElement(SPElement *parent, SPTableDataSource *dataSource)
    {
    	this->setTagName("table");
        //ctor
        this->dataSource = dataSource;
        this->addEventListener("mousedown", SPCallback(&SPTableElement::mouseDown), this, this);
    }
    SPTableElement::~SPTableElement()
    {
        //dtor
    }
    string SPTableElement::getType() {
        return "table";
    }
    void SPTableElement::paint(int x, int y, SPGraphicsContext *g) {
        std::vector<SPNode *> children = this->getChildNodes();
        x += this->getX() ;
        y += this->getY();
        this->absoluteBounds = {
	        x + this->getMargins().left, y + this->getMargins().top, this->getWidth(), this->getHeight()
        };

        int width =  this->getWidth();
        int height = this->getHeight();

        y = y + 1;
        SPTableDataSource *dataSource = this->dataSource;

        // Draw table data here
        width = this->getWidth();
        if (width < 1) {
            width = (this->getParentElement())->getWidth();
        }
        int columnY = y;

        int left = 0;
        int padding = 15;
        int columnWidth = width / dataSource->getColumnCount();
        const int rowH = 18;
        SPColor fgColor = this->getForegroundColor();

        // Draw column headers first (sticky at columnY)
        if (columnY < 60) {
            columnY = this->getParentElement()->getAbsoluteBounds().y + 1;
        }
        m_firstRowY = columnY + rowH;  // remember for hit-testing in mouseDown
        this->getSkin()->drawTableHeader(g, this, x, columnY, width, rowH);
        left = 0;
        for (int i = 0; i < dataSource->getColumnCount(); i++) {
            string text = dataSource->getTextForColumn(i);
            g->drawString(text, SPFontStyle("Tahoma", 13, 1, true, false), fgColor, x + padding + left, columnY + 1, columnWidth, rowH);
            g->drawString(text, SPFontStyle("Tahoma", 13, 1, false, false), fgColor.lighten(-10), x + padding + left, columnY, columnWidth, rowH);
            left += columnWidth;
        }

        // Draw row backgrounds (zebra stripe) then row text
        int rowY = columnY + rowH;
        for (int i = 0; i < dataSource->getRowCount(); i++) {
            this->getSkin()->drawTableRow(g, this, i, x, rowY + i * rowH, width, rowH, i);
        }

        // Full-view mode: extend zebra stripes into the empty area below the last row
        if (this->fullView) {
            int parentBottom = this->getParentElement()->getAbsoluteBounds().y
                             + this->getParentElement()->getHeight();
            int emptyY   = rowY + dataSource->getRowCount() * rowH;
            int ghostRow = dataSource->getRowCount();
            if (gradientView) {
                // Draw a gradient in the empty area instead of zebra stripes
                SPGradient gradient(
                    this->getBackgroundColor().lighten(0.5), this->getBackgroundColor().lighten(0),
                    0
                );
                g->fillRoundedLinearGradientRectangle(x, emptyY, width, parentBottom - emptyY, 0, 0, gradient);
            } else {
                while (emptyY < parentBottom) {
                    this->getSkin()->drawTableRow(g, this, ghostRow, x, emptyY, width, rowH, ghostRow);
                    emptyY   += rowH;
                    ghostRow += 1;
                }
            }
        }

        rowY = columnY + rowH;
        const SPColor selFgColor(95, 132, 180, 255);
        const SPColor unplayableDark(163, 100, 105, 255);   // #e3c8d4
        const SPColor unplayableLight(105,   0,  46, 255);  // #69002e
        const bool isDark = (this->getColorScheme() == SP_COLOR_SCHEME_DARK);
        const int numCols = dataSource->getColumnCount();
        for (int i = 0; i < dataSource->getRowCount(); i++) {
            SPColor rowFg = fgColor;
            auto *app = SPApplicationContext::current();
            if (!dataSource->isRowPlayable(i)) {
                rowFg = isDark ? unplayableDark : unplayableLight;
            }
            if (app && app->hasCurrentPlayable()) {
                if (i < dataSource->getRowCount()) {
                    if (dataSource->getRowUri(i) == app->currentPlayableUri()) {
                        rowFg = SPColor(50, 255, 50, 255);
                    }
                }
            }
            if (isRowSelected(i)) {
                rowFg = selFgColor;
            } 
            float prog = dataSource->getProgressForRow(i);
            for (int j = 0; j < numCols; j++) {
                // Last column: draw a progress bar when progress >= 0
                if (prog >= 0.f && j == numCols - 1) {
                    int barX = x + j * columnWidth + padding;
                    int barW = columnWidth - padding * 2;
                    int barY = rowY + 4;
                    int barH = rowH - 8;
                    // Track background
                    if (!gradientView) {
                        g->fillRectangle(barX, barY, barW, barH, SPColor(80, 80, 80, 120));
                    }
                    // Filled portion
                    int filled = std::max(0, static_cast<int>(barW * prog));
                    if (filled > 0)
                        g->fillRectangle(barX, barY, filled, barH, SPColor(60, 120, 220, 220));
                } else {
                    string text = dataSource->getTextForCell(j, i);
                    left = x + (j * columnWidth) + padding;
                    g->drawString(text, SPFontStyle("Tahoma", 13, 1, false, false), rowFg, left, rowY + 1, columnWidth, rowH);
                }
            }
            rowY += rowH;
        }
    }

    // ── Mouse click → row selection + double-click / right-click detection ──────
    void SPTableElement::mouseDown(SPObject* sender, SPEventArgs* e) {
        auto *me = dynamic_cast<SPMouseEventArgs *>(e);
        if (!me || !dataSource) return;

        int relY   = me->getY() - m_firstRowY;
        int rowIdx = relY / kRowH;
        int rowCnt = dataSource->getRowCount();

        if (rowIdx < 0 || rowIdx >= rowCnt) return;

        // ── Right-click → select row + fire "rowrightclicked" ─────────────
        if (me->getButton() == 1) {
            m_selectedRows.clear();
            m_selectedRows.insert(rowIdx);
            m_anchorRow = rowIdx;
            this->invalidate();
            auto* args = new SPTableRowEventArgs(rowIdx);
            this->dispatchEvent("rowrightclicked", args, this);
            delete args;
            return;
        }

        // ── Double-click detection ─────────────────────────────────────────
        auto now = ClockType::now();
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - m_lastClickTime).count();

        if (rowIdx == m_lastClickRow && elapsedMs < kDblClickMs) {
            // Double-click confirmed on same row → dispatch "rowdblclicked"
            auto* dblArgs = new SPTableRowEventArgs(rowIdx);
            this->dispatchEvent("rowdblclicked", dblArgs, this);
            delete dblArgs;
            m_lastClickRow  = -1;
            m_lastClickTime = ClockType::time_point{};
        } else {
            m_lastClickRow  = rowIdx;
            m_lastClickTime = now;
        }

        // ── Selection update ───────────────────────────────────────────────
        if (me->getCmdDown()) {
            if (m_selectedRows.count(rowIdx))
                m_selectedRows.erase(rowIdx);
            else
                m_selectedRows.insert(rowIdx);
        } else {
            m_selectedRows.clear();
            m_selectedRows.insert(rowIdx);
        }
        m_anchorRow = rowIdx;
        this->invalidate();
    }

    // ── Keyboard navigation ───────────────────────────────────────────────────
    void SPTableElement::processEvent(const SPEvent& event) {
        if (event.type == "keydown") {
            auto *ke = dynamic_cast<SPKeyEventArgs *>(event.args);
            if (ke && dataSource) {
                int rowCnt = dataSource->getRowCount();
                if (rowCnt == 0) return;

                int next = m_anchorRow;
                if (ke->getKeyCode() == SPKeyEventArgs::KeyUp)
                    next = (m_anchorRow <= 0) ? 0 : m_anchorRow - 1;
                else if (ke->getKeyCode() == SPKeyEventArgs::KeyDown)
                    next = (m_anchorRow + 1 >= rowCnt) ? rowCnt - 1 : m_anchorRow + 1;
                else
                    return;

                m_selectedRows.clear();
                m_selectedRows.insert(next);
                m_anchorRow = next;
                this->invalidate();
            }
            return;
        }
        SPElement::processEvent(event);
    }
}
