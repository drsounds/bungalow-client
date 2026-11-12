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
#include <cstdlib>
#include <utility>

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
        this->addEventListener("mousemove", SPCallback(&SPTableElement::mouseMove), this, this);
        this->addEventListener("mouseup", SPCallback(&SPTableElement::mouseUp), this, this);
        SPDragDrop::registerTarget(this, this);
    }
    SPTableElement::~SPTableElement()
    {
        SPDragDrop::unregisterTarget(this);
        if (m_pressed && s_capturedElement == this) releaseMouseCapture();
    }
    string SPTableElement::getType() {
        return "table";
    }
    void SPTableElement::paint(int x, int y, SPGraphicsContext *g) {
        std::vector<SPNode *> children = this->getChildNodes();
        x += this->getX() ;
        y += this->getY();
        this->absoluteBounds = {
	        x, y, this->getWidth(), this->getHeight()
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
        m_paintX = x;
        m_columnWidth = columnWidth;
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
                // Let the skin fill the empty area; otherwise draw a gradient instead of zebra stripes
                const sp_rectangle empty { x, emptyY, width, parentBottom - emptyY, 0 };
                if (!this->getSkin()->drawElement(g, this, "", empty)) {
                    SPGradient gradient(
                        this->getBackgroundColor().lighten(0.5), this->getBackgroundColor().lighten(0),
                        0
                    );
                    g->fillRoundedLinearGradientRectangle(x, emptyY, width, parentBottom - emptyY, 0, 0, gradient);
                }
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
                    left = x + (j * columnWidth) + padding;
                    const SPFontStyle cellFont("Tahoma", 13, 1, false, false);
                    const CellKey key(i, j);
                    const string markup = dataSource->getMarkupForCell(j, i);
                    if (markup.empty()) {
                        if (!m_cells.empty()) m_cells.erase(key);
                        string text = dataSource->getTextForCell(j, i);
                        g->drawString(text, cellFont, rowFg, left, rowY + 1, columnWidth, rowH);
                    } else {
                        // Links keep the row's color, so selected, playing and
                        // unplayable rows still read as such.
                        SPTableCell &cell = m_cells[key];
                        cell.setMarkup(markup);
                        cell.paint(g, cellFont, { left, rowY + 1, columnWidth - padding, rowH - 1, 0 },
                                   rowFg, rowFg, key == m_hoverCell ? m_hoverLinkUri : string());
                    }
                }
            }
            rowY += rowH;
        }
        // Forget the cells of rows that are gone.
        if (!m_cells.empty())
            m_cells.erase(m_cells.lower_bound(CellKey(dataSource->getRowCount(), 0)), m_cells.end());

        if (m_dropIndex >= 0) {
            const int lineY = m_firstRowY + m_dropIndex * rowH;
            g->fillRectangle(x, lineY - 1, width, 2, SPColor(99, 149, 234, 255));
        }
    }

    int SPTableElement::rowAtY(int absoluteY) const {
        const int row = (absoluteY - m_firstRowY) / kRowH;
        return row >= 0 && dataSource && row < dataSource->getRowCount() ? row : -1;
    }

    int SPTableElement::dropIndexAtY(int absoluteY) const {
        if (!dataSource) return 0;
        const int rowCount = dataSource->getRowCount();
        const int offset = absoluteY - m_firstRowY;
        if (offset <= 0) return 0;
        const int row = offset / kRowH;
        if (row >= rowCount) return rowCount;
        return row + ((offset % kRowH) >= kRowH / 2 ? 1 : 0);
    }

    // ── Links ────────────────────────────────────────────────────────────────
    const SPTableCell *SPTableElement::cellAt(int x, int y, CellKey *key) const {
        if (m_cells.empty() || m_columnWidth <= 0 || x < m_paintX) return nullptr;
        const int row = rowAtY(y);
        if (row < 0) return nullptr;
        const CellKey found(row, (x - m_paintX) / m_columnWidth);
        const auto it = m_cells.find(found);
        if (it == m_cells.end()) return nullptr;
        if (key) *key = found;
        return &it->second;
    }

    string SPTableElement::linkAt(int x, int y) const {
        const SPTableCell *cell = cellAt(x, y);
        return cell ? cell->uriAt(x, y) : string();
    }

    void SPTableElement::clearLinkHover() {
        m_hoverCell = CellKey(-1, -1);
        SPElement::clearLinkHover();
    }

    // ── Mouse click → row selection + double-click / right-click detection ──────
    void SPTableElement::mouseDown(SPObject* sender, SPEventArgs* e) {
        auto *me = dynamic_cast<SPMouseEventArgs *>(e);
        if (!me || !dataSource) return;

        // A left press on a link belongs to the link: it is followed when the
        // button comes up on it again, and does not select or drag the row.
        m_pressedLink.clear();
        if (me->getButton() == 0) {
            CellKey key;
            const SPTableCell *cell = cellAt(me->getX(), me->getY(), &key);
            const string link = cell ? cell->uriAt(me->getX(), me->getY()) : string();
            if (!link.empty()) {
                m_pressedLink = link;
                m_pressedCell = key;
                return;
            }
        }

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
        } else if (!m_selectedRows.count(rowIdx)) {
            m_selectedRows.clear();
            m_selectedRows.insert(rowIdx);
        }
        m_anchorRow = rowIdx;
        this->invalidate();

        // A press on a selected row may turn into a drag. Capture the mouse so
        // the release is seen even when it happens outside the table.
        if (m_selectedRows.count(rowIdx)) {
            m_pressed = true;
            m_pressX  = me->getX();
            m_pressY  = me->getY();
            setMouseCapture(this);
        }
    }

    // ── Drag source ───────────────────────────────────────────────────────────
    void SPTableElement::mouseMove(SPObject* /*sender*/, SPEventArgs* e) {
        auto *me = dynamic_cast<SPMouseEventArgs *>(e);
        if (!me) return;
        if (!m_pressed) {
            CellKey key;
            const SPTableCell *cell = cellAt(me->getX(), me->getY(), &key);
            const string link = cell ? cell->uriAt(me->getX(), me->getY()) : string();
            if (!link.empty()) {
                if (key != m_hoverCell) {
                    m_hoverCell = key;
                    this->invalidate();
                }
                this->claimLinkHover(link);
            }
            return;
        }
        if (std::abs(me->getX() - m_pressX) < kDragThreshold &&
            std::abs(me->getY() - m_pressY) < kDragThreshold)
            return;
        startDrag();
    }

    void SPTableElement::mouseUp(SPObject* /*sender*/, SPEventArgs* e) {
        if (!m_pressedLink.empty()) {
            const string pressed = m_pressedLink;
            m_pressedLink.clear();
            auto *me = dynamic_cast<SPMouseEventArgs *>(e);
            CellKey key;
            const SPTableCell *cell = me ? cellAt(me->getX(), me->getY(), &key) : nullptr;
            if (cell && me->getButton() == 0 && key == m_pressedCell &&
                cell->uriAt(me->getX(), me->getY()) == pressed)
                this->activateLink(pressed);   // may refill or hide the table
            return;
        }
        if (!m_pressed) return;
        m_pressed = false;
        if (s_capturedElement == this) releaseMouseCapture();
    }

    void SPTableElement::startDrag() {
        m_pressed = false;
        if (s_capturedElement == this) releaseMouseCapture();
        if (!dataSource) return;

        SPDragData data;
        data.uris = dataSource->getDragUris(getSelectedRows());
        if (data.uris.empty()) return;

        // The OS runs the drag and takes the button release with it, so the
        // pressed state the mouse-down left on the way here is undone by hand.
        SPDragDrop::start(this, std::move(data));
        for (SPElement* element = this; element; element = element->getParentElement())
            element->setState(SP_STATE_NORMAL);
    }

    // ── Drop target ───────────────────────────────────────────────────────────
    bool SPTableElement::dragOver(int /*x*/, int y, const SPDragData& data) {
        if (!m_dropFilter || !m_dropHandler || !m_dropFilter(data)) {
            setDropIndex(-1);
            return false;
        }
        setDropIndex(dropIndexAtY(y));
        return true;
    }

    void SPTableElement::dragLeave() {
        setDropIndex(-1);
    }

    bool SPTableElement::drop(int /*x*/, int y, const SPDragData& data) {
        setDropIndex(-1);
        if (!m_dropFilter || !m_dropHandler || !m_dropFilter(data)) return false;
        return m_dropHandler(data, dropIndexAtY(y));
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
