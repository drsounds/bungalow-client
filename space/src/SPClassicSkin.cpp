//
// Created by Alexander Forselius on 2025-12-11.
//

#include "SPClassicSkin.h"

#include "SPGraphicsContext.h"
#include "SPTableDataSource.h"
#include "libspace.h"
#include "SPAppHeaderElement.h"
#include "SPButtonElement.h"
#include "SPApplicationContext.h"
#include "SPColor.h"

#include <cmath>
#include <iostream>

using namespace space;

bool SPClassicSkin::drawElement(SPGraphicsContext* g, SPElement *elm, const string& state, const sp_rectangle& bounds)
{
    bool result = false;
    SPColor bgColor = elm->getBackgroundColor();
    SPColor fgColor = elm->getForegroundColor();
    SPColor shadowColor = this->m_shadowColor;
    std::optional<SPColor> tintColor = elm->getTintColor();
    if (tintColor.has_value())
    {
        bgColor = bgColor.tint(tintColor.value());
        fgColor = fgColor.tint(tintColor.value());
        shadowColor = tintColor.value().lighten(-0);
    }
    if (elm->getType() == "input") {
        result = true;
    }
    if (elm->getType() == "splitter") {
        result = true;
        bgColor = shadowColor;
        g->fillRectangle(bounds.x, bounds.y, bounds.width, bounds.height, bgColor);
    }
    if (elm->getType() == "button" || elm->getType() == "nowplayingbarheader" || elm->getType() == "nowplayingbartitle")
    {
        if (elm->getType() == "nowplayingbarheader" || elm->getType() == "nowplayingbartitle") {
            cout << "Now Playing Bar element: " << elm->getType() << endl;
        }
        result = true;
        int kR = 25;
    	if (elm->getType() == "nowplayingbarheader" || elm->getType() == "nowplayingbartitle") {
    		kR = 0;
        }
        SPColor bgColor = elm->getBackgroundColor(); 
        if (elm->getStringAttribute("variant", "default") == "primary") {
            bgColor = tintColor.value().alpha(1);
        } else {
            bgColor = bgColor.tint(tintColor.value()).alpha(1);
        }   
    	if (elm->getColorScheme() == SP_COLOR_SCHEME_DARK)
    		bgColor = bgColor.lighten(10);
        if (state == "active")
            bgColor = bgColor.lighten(-0.15f);
        else if (state == "hover")
            bgColor = bgColor.lighten(0.05f);

        /*switch (m_buttonStyle) {
        case kButtonFlat: {
            g->fillRoundedRectangle(
                bounds.x, bounds.y, bounds.width, bounds.height, kR,
                SPColor(bgColor.getR(), bgColor.getG(), bgColor.getB(), 230));
            break;
        }
        case kButtonGradient: {*/
            SPGradient grad(bgColor.lighten(0.50f), bgColor.lighten(-0.40f), 0);
            if (state == "active")
                grad = SPGradient(grad.getEnd(), grad.getStart(), 0);
            g->fillRoundedLinearGradientRectangle(
                bounds.x, bounds.y, bounds.width, bounds.height, kR, 0, grad);
            /*break;
        }
        case kButtonModernGradient: {
            float h, s, l;
            bgColor.toHSL(h, s, l);
            SPColor sec = SPColor::fromHSL(h + 60.0f, s * 0.85f, l * 0.85f, bgColor.getA());
            SPGradient grad(bgColor, sec, 0);
            g->fillRoundedLinearGradientRectangle(
                bounds.x, bounds.y, bounds.width, bounds.height, kR, 0, grad);
            break;
        }
        case kButtonAqua: {
            g->fillRoundedRectangle(
                bounds.x, bounds.y, bounds.width, bounds.height, kR,
                SPColor(bgColor.getR(), bgColor.getG(), bgColor.getB(), 230));
            int topH = bounds.height * 2 / 5;
            SPGradient hiSpec(SPColor(255, 255, 255, 165), SPColor(255, 255, 255, 0), 0);
            g->fillRoundedLinearGradientRectangle(
                bounds.x + 3, bounds.y + 2, bounds.width - 6, topH, kR, 0, hiSpec);
            int botH = std::max(bounds.height / 7, 3);
            SPGradient botSpec(SPColor(255, 255, 255, 0), SPColor(255, 255, 255, 45), 0);
            g->fillRoundedLinearGradientRectangle(
                bounds.x + 2, bounds.y + bounds.height - botH - 2,
                bounds.width - 4, botH, 3, 0, botSpec);
            break;
        }
        }*/

    	if (this->m_glossy && state != "active") {
    		SPGradient gradient2(bgColor.lighten(0.2f), bgColor.lighten(0.3f), 0);
    		g->fillRoundedLinearGradientRectangle(
				bounds.x + 5, bounds.y + 2,
				bounds.width - 10, bounds.height / 2,
				kR, 0, gradient2);
    		SPGradient gradient3(bgColor.lighten(-0.2f).alpha(0), bgColor.lighten(0.0f), 0);
    		g->fillRoundedLinearGradientRectangle(
				bounds.x + 10, bounds.y + bounds.height / 2 - 2,
				bounds.width - 20, bounds.height / 2,
				kR, 0, gradient3);
    	}
    	g->drawRoundedRectangle(
			bounds.x, bounds.y, bounds.width, bounds.height,
			kR, bgColor.lighten(-0.5f));

        if(!elm->getInnerText().empty()) {
            SPGraphicsContext *c = elm->createGraphics();
            space::SPFontStyle fontStyle = elm->getFontStyle();
            string text = elm->getInnerText();
            space::sp_rectangle tSize = c->measureString(text, fontStyle);
            int x = (elm->getWidth() / 2) - (tSize.width / 2) + elm->getAbsoluteBounds().x;
            int y = (elm->getHeight() / 2) - (tSize.height / 2) + elm->getAbsoluteBounds().y;

            c->drawString(text,fontStyle, SPColor(10, 10, 10, 255), x, y, tSize.width, tSize.height);
        }
    }
    if (elm->getType() == "table")
    {

    }
    if (elm->getType() == "appheader")
    {
        result = true;
        // Re-get fresh background color (same pattern as button) to avoid
        // double-tinting from the outer bgColor computation at the top.
        SPColor bgColor = elm->getBackgroundColor();
        if (elm->getColorScheme() == SP_COLOR_SCHEME_DARK)
            bgColor = bgColor.lighten(2);
        SPGradient gradient(bgColor.lighten(0.1), bgColor.lighten(-0.1), 0);
        g->drawLinearGradient({ bounds.x, bounds.y, bounds.width, bounds.height }, gradient);
    	if (this->m_glossy && state != "active") {
    		SPGradient gradient2(bgColor.lighten(0.2), bgColor.lighten(0), 0);
    		g->drawLinearGradient({ bounds.x, bounds.y + 1, bounds.width, bounds.height / 2 }, gradient2);
    	}
    }
    if (elm->getType() == "appfooter")
    {
        result = true;
        SPGradient gradient(
            bgColor.lighten(0.2), bgColor.lighten(0.1),
            0
        );
        g->drawLinearGradient(
    {
                bounds.x,
                bounds.y,
                bounds.width,
                bounds.height
            },
            gradient
        );
    	if (this->m_glossy && state != "active") {
    		SPGradient gradient2(
				bgColor.lighten(0.2), bgColor.lighten(0.3),
				0
				);
    		g->drawLinearGradient(
				{
					bounds.x,
					bounds.y + 2,
					bounds.width,
					bounds.height / 2,
				},
				gradient2
			);
    		SPGradient gradient3(
				bgColor.lighten(-0.2).alpha(0), bgColor.lighten(0),
				0
			);
    		g->drawLinearGradient(
				{
				bounds.x,
				bounds.y + bounds.height / 2 - 2,
				bounds.width,
				bounds.height / 2,
				},
				gradient3
			);
    	}
    }
    if (elm->getType() == "treeview")
    {
    	if (elm->getColorScheme() == SP_COLOR_SCHEME_DARK) {
    		bgColor = bgColor.lighten(0.28);
    	}
        result = true;
        auto *treeView = dynamic_cast<SPTreeViewElement *>(elm);
        auto width = treeView->getWidth();
        auto height = treeView->getHeight();
        SPGradient gradient(
            bgColor.lighten(0.5), bgColor.lighten(0),
            0
        );
        if (state == "active")
        {
            gradient = SPGradient(SPColor(0, 0, 0, 1), SPColor(10, 10, 10, 0), 0);
        }
        g->fillRectangle(bounds.x, bounds.y, bounds.width, bounds.height, bgColor);
    }
    if (elm->getType() == "viewstack")
    {
        result = true;
        g->fillRectangle(bounds.x, bounds.y, bounds.width, bounds.height, bgColor);
    }
    return result;
}

void SPClassicSkin::drawTableRow(SPGraphicsContext *g, SPTableElement *table, int i, int x, int y, int width, int height, int row)
{
    if (table->isRowSelected(row)) {
        g->fillRectangle(x, y, width, height, SPColor(166, 218, 251, 255));
    } else {
        if (row % 2 == 0) {
            SPColor bgColor = table->getBackgroundColor();
            if (auto tintColor = table->getTintColor(); tintColor)
                bgColor = bgColor.tint(tintColor.value_or(SPColor(200, 200, 200, 255)));
            // Semi-transparent darker tint for even rows
            SPColor stripeColor = bgColor.lighten(-0.2).alpha(100);
            if (!table->gradientView) {
                g->fillRectangle(x, y, width, height, stripeColor);
            }
        }
        auto* app = SPApplicationContext::current();
        if (app != nullptr) {
            if (app->hasCurrentPlayable() && table->dataSource) {
                auto* dataSource = table->dataSource;
                if (row < dataSource->getRowCount()) {
                    if (dataSource->getRowUri(row) == app->currentPlayableUri()) {
                        g->fillRectangle(x, y, width, height, SPColor(0, 0, 0, 255));
                    }
                }
            }
        }
    }
}

void SPClassicSkin::drawTableHeader(SPGraphicsContext* g, SPTableElement *table, int x, int y, int width, int height)
{
    SPColor bgColor = table->getBackgroundColor();
    if (auto tintColor = table->getTintColor(); tintColor.has_value())
        bgColor = bgColor.tint(tintColor.value());
    if (table->getColorScheme() == SP_COLOR_SCHEME_DARK)
        bgColor = bgColor.lighten(2);

    int colCount = (table->dataSource && table->dataSource->getColumnCount() > 0)
        ? table->dataSource->getColumnCount() : 1;
    int colWidth = width / colCount;

    // Draw each column header cell like a button (default gradient + gloss)
    for (int col = 0; col < colCount; ++col) {
        int cx = x + col * colWidth;
        int cw = (col == colCount - 1) ? (width - col * colWidth) : colWidth;

        // Base button gradient: lighter top → darker bottom
        SPGradient gradient(bgColor.lighten(0.1), bgColor.lighten(-0.2), 0);
        g->drawLinearGradient({cx, y, cw, height}, gradient);

        // Gloss upper highlight
        if (m_glossy) {
            SPGradient gloss(bgColor.lighten(0.25), bgColor.lighten(0.15), 0);
            g->drawLinearGradient({cx + 2, y + 1, cw - 4, height / 2}, gloss);
            SPGradient gloss2(bgColor.lighten(-0.15).alpha(0), bgColor.lighten(0.05), 0);
            g->drawLinearGradient({cx + 2, y + height / 2, cw - 4, height / 2 - 1}, gloss2);
        }

        // Column divider (shadow line on right edge, light line inset)
        SPColor divider = bgColor.lighten(-1.5);
        SPColor dividerLight = bgColor.lighten(1.3);
        if (col < colCount - 1) {
            g->drawLine(cx + cw - 1, y, cx + cw - 1, y + height - 1, divider);
            //g->drawLine(cx + cw,     y, cx + cw,     y + height - 1, dividerLight);
        }
    }

    // Bottom border — same shadow line as button border
    SPColor borderColor = bgColor.lighten(-0.5);
    g->drawLine(x, y + height - 1, x + width, y + height - 1, borderColor);
}

void SPClassicSkin::drawTabBarToolButton(SPGraphicsContext* g, SPTabBarElement* tabBar, SPTabToolButton* btn, int x, int y, int width, int height)
{
    SPColor bgColor = tabBar->getBackgroundColor();
    std::optional<SPColor> tintColor = tabBar->getTintColor();
    if (tintColor.has_value())
        bgColor = bgColor.tint(tintColor.value());
    if (tabBar->getColorScheme() == SP_COLOR_SCHEME_DARK)
        bgColor = bgColor.lighten(2);

    // Subtle inset button appearance matching the tab bar style
    SPGradient grad(bgColor.lighten(0.05f), bgColor.lighten(-0.15f), 0);
    g->drawLinearGradient({x + 1, y + 2, width - 2, height - 4}, grad);

    // Border
    SPColor border = this->m_shadowColor;
    if (tintColor.has_value()) border = tintColor.value().lighten(-0.3f);
    g->drawRoundedRectangle(x + 1, y + 2, width - 2, height - 4, 20, border);

    // Vertical divider on the left
    g->drawLine(x, y, x, y + height - 1, border);
}

void SPClassicSkin::drawTabBarTab(SPGraphicsContext* g, SPTabBarElement *tabBar, SPTab* tab, int index, bool active, int x, int y, int width, int height)
{
    SPColor bgColor = tabBar->getBackgroundColor();
    SPColor activeBgColor = bgColor; // preserve original for active tab
    SPColor lColor = this->m_lightColor;
    SPColor shadowColor = this->m_shadowColor;
    std::optional<SPColor> tintColor = tabBar->getTintColor();
    if (tintColor.has_value())
    {
        bgColor = bgColor.tint(tintColor.value());
        activeBgColor = bgColor;
        shadowColor = tintColor.value().lighten(-0);
        lColor = lColor.tint(tintColor.value()).lighten(0.5);
    }
	if (tabBar->getColorScheme() == SP_COLOR_SCHEME_DARK) {
		bgColor = bgColor.lighten(2);
		shadowColor = shadowColor.lighten(-1.7);
	}
    if (active)
    {
        g->fillRectangle(x, y, width, height, activeBgColor);
        if (index > 0)
        {
            g->drawLine(x, y, x, y + height - 1,shadowColor);
        } else {

            g->drawLine(x + width - 1, y, x + width - 1, y + height - 1,shadowColor);
        }
    }
    else
    {
		//g->drawLine(x, y + 1, x + width, y + 1, lColor);
		//g->drawLine(x, y + 1, x, y + height - 2, lColor);
		//g->drawLine(x + width + 1, y + 1, x + width + 1, y + height - 2, lColor);
    }
    g->drawLine(x + width, y, x + width, y + height - 1,shadowColor);
}

void SPClassicSkin::drawTabBar(
    SPGraphicsContext* g,
    SPTabBarElement* tabBar,
    int x,
    int y,
    int width,
    int height
)
{
    SPColor lColor = this->m_lightColor;
    SPColor shadowColor = this->m_shadowColor;
    SPColor bgColor = tabBar->getBackgroundColor();
    std::optional<SPColor> tintColor = tabBar->getTintColor();
    if (tintColor.has_value())
    {
        bgColor = bgColor.tint(tintColor.value());
        lColor = lColor.tint(tintColor.value()).lighten(0.5);
        shadowColor = tintColor.value().lighten(-0);
    }
	if (tabBar->getColorScheme() == SP_COLOR_SCHEME_DARK) {
		bgColor = bgColor.lighten(2);
		shadowColor = shadowColor.lighten(-0.5);
	}
    SPGradient gradient(
        bgColor.lighten(0.1), bgColor.lighten(-0.1),
        0
        );
    g->drawLinearGradient(sp_rectangle {x, y, width, height}, gradient);
    /*if (this->m_glossy) {
    	SPGradient gradient2(
			bgColor.lighten(0.2), bgColor.lighten(0),
			0
		);
    	g->drawLinearGradient(sp_rectangle {x, y + 1, width, height / 2}, gradient2);
    }
    if (this->getButtonStyle() == kButtonAqua) {
        SPGradient hiSpec(SPColor(255, 255, 255, 165), SPColor(255, 255, 255, 0), 0);
        g->drawLinearGradient(sp_rectangle {x + 3, y + 2, width - 6, height / 2}, hiSpec);
        int botH = std::max(height / 7, 3);
        SPGradient botSpec(SPColor(255, 255, 255, 0), SPColor(255, 255, 255, 45), 0);
        g->drawLinearGradient(sp_rectangle {x + 2, y + height - botH - 2, width - 4, botH}, botSpec);
    } else if (this->getButtonStyle() == kButtonGradient) {
        SPGradient grad(bgColor.lighten(0.10f), bgColor.lighten(-0.20f), 0);
        g->drawLinearGradient(sp_rectangle {x, y, width, height}, grad);
    }*/
    //g->drawLine(x, y, x + width, y,shadowColor);
    //g->drawLine(x, y + 1, x + width, y + 1, lColor);
    g->drawLine(x, y + height - 1, x + width, y + height - 1, shadowColor);
}

void SPClassicSkin::drawTreeItem(SPGraphicsContext* g, SPTreeViewElement *treeView, SPTreeItem* item, int itemHeight, int x, int y, int width, int height)
{
    if (item->isSelected()) {
        g->fillRectangle(x, y, treeView->getWidth(), itemHeight, SPColor(166, 218, 251, 255));
    }
    if (item->isHighlighted()) {
        g->fillRectangle(x, y, treeView->getWidth(), itemHeight, SPColor(166, 218, 251, 128));
    }
}

void SPClassicSkin::drawSectionHeader(SPGraphicsContext* g, SPTreeViewElement* treeView, const string& title, int x, int y, int width, int height)
{
    // Muted label — slightly smaller font, uppercase appearance via color
    SPFontStyle sectionStyle(
        treeView->getFontStyle().getFamily(),
        treeView->getFontStyle().getHeight() - 1,
        treeView->getFontStyle().getWeight(),
        false, false
    );
    SPColor labelColor(128, 128, 128, 200);
    g->drawString(title, sectionStyle, labelColor, x + 8, y + 4, width - 8, height);
}

SPClassicSkin::SPClassicSkin() {
}
