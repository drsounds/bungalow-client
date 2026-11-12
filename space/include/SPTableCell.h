#ifndef TABLECELL_H
#define TABLECELL_H
#include "SPRichText.h"

#include <string>
using namespace std;
namespace space {
    class SPColor;
    class SPGraphicsContext;
    class SPTableRow;
    class SPTableElement;
/**
 * A table cell whose text is markup (see SPRichText), so it can hold links,
 * images and bold, italic or underlined text. The table keeps one for every
 * such cell it has painted, and asks it which link is under the pointer.
 */
class SPTableCell
{
    public:
        SPTableCell() = default;
        SPTableCell(SPTableRow *tableRow);
        virtual ~SPTableCell();
        string uri;
        string title;
        SPTableRow *tableRow = nullptr;
        SPTableElement *tableElement = nullptr;

        void setMarkup(const string& markup) { m_text.setMarkup(markup); }
        const string& getMarkup() const { return m_text.getMarkup(); }
        bool hasLinks() const { return m_text.hasLinks(); }

        /// Draws the cell's text at the top left corner of bounds, in window
        /// coordinates, and remembers bounds for uriAt(). Links are drawn in
        /// linkColor and the one to hoverUri is underlined.
        void paint(SPGraphicsContext *g, const SPFontStyle& font, const sp_rectangle& bounds,
                   const SPColor& color, const SPColor& linkColor, const string& hoverUri);
        /// The link at a point in window coordinates, or "". Valid after paint().
        string uriAt(int x, int y) const;
    protected:
    private:
        SPRichText m_text;
        sp_rectangle m_bounds { 0, 0, 0, 0, 0 };
};
}

#endif // TABLECELL_H
