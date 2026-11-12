#include "SPTableCell.h"

#include "SPColor.h"
#include "SPGraphicsContext.h"
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

void SPTableCell::paint(SPGraphicsContext *g, const SPFontStyle& font, const sp_rectangle& bounds,
                        const SPColor& color, const SPColor& linkColor, const string& hoverUri)
{
    m_bounds = bounds;
    m_text.paint(g, font, bounds.x, bounds.y, color, linkColor, hoverUri);
}

string SPTableCell::uriAt(int x, int y) const
{
    // Text may run past the cell; only what is inside it can be clicked.
    if (x < m_bounds.x || x >= m_bounds.x + m_bounds.width ||
        y < m_bounds.y || y >= m_bounds.y + m_bounds.height)
        return string();
    return m_text.uriAt(x - m_bounds.x, y - m_bounds.y);
}
}
