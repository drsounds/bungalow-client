//
// Created by Alexander Forselius on 2026-03-21.
//

#include "SPModernSkin.h"

#include "SPGraphicsContext.h"
#include "SPColor.h"
#include <cmath>
#include "SPElement.h"
#include "SPAppHeaderElement.h"
#include "SPButtonElement.h"
#include "SPTreeViewElement.h"
#include "SPTableElement.h"
#include "SPTableDataSource.h"
#include "libspace.h"

using namespace space;

// Corner radius for the modern liquid glass look — tighter than the classic 25px pill.
static const int kModernRadius = 10;

// Separator color appropriate for the current color scheme.
static SPColor sepColor(short scheme)
{
    return scheme == SP_COLOR_SCHEME_DARK
        ? SPColor(255, 255, 255, 30)
        : SPColor(0, 0, 0, 25);
}

// Resolve an element's background color with optional tint and dark-mode boost.
static SPColor resolveBackground(SPElement* elm, std::optional<SPColor> tint)
{
    SPColor bg = elm->getBackgroundColor();
    if (tint) bg = bg.tint(*tint);
    if (elm->getColorScheme() == SP_COLOR_SCHEME_DARK) bg = bg.lighten(2);
    return bg;
}

// Draws a liquid-glass specular + refraction pass over an element.
// Specular: bright highlight along the top portion (light reflecting off glass).
// Refraction: subtle brightening along the bottom (back-lit translucency).
static void drawGlassSheen(SPGraphicsContext* g, int x, int y, int w, int h, int r)
{
    int topH = h * 2 / 5;
    int botH = h - topH;

    // Specular highlight: white fading from ~35% opacity at top to transparent at centre
    SPGradient specular(
        SPColor(255, 255, 255, 90),  // ~35% white
        SPColor(255, 255, 255, 0),   // transparent
        0
    );
    g->fillRoundedLinearGradientRectangle(x, y, w, topH, r, 0, specular);

    // Refraction tint: transparent at centre to faint white at bottom
    SPGradient refraction(
        SPColor(255, 255, 255, 0),   // transparent
        SPColor(255, 255, 255, 26),  // ~10% white
        0
    );
    g->fillRoundedLinearGradientRectangle(x, y + topH, w, botH, r, 0, refraction);
}

bool SPModernSkin::drawElement(SPGraphicsContext* g, SPElement* elm, const string& state, const sp_rectangle& bounds)
{
    bool result = false;
    SPColor bgColor = elm->getBackgroundColor();
    std::optional<SPColor> tintColor = elm->getTintColor();
    if (tintColor) {
        bgColor = bgColor.tint(*tintColor);
    }

    if (elm->getType() == "input") {
        result = true;
    }

    if (elm->getType() == "splitter") {
        result = true;
        g->fillRectangle(bounds.x, bounds.y, bounds.width, bounds.height,
                         sepColor(elm->getColorScheme()));
    }

    if (elm->getType() == "button") {
        result = true;

        SPColor btnBg = elm->getBackgroundColor();
        if (tintColor) {
            if (elm->getStringAttribute("variant", "default") == "primary") {
                btnBg = (*tintColor).alpha(1);
            } else {
                btnBg = btnBg.tint(*tintColor);
            }
        }
        if (elm->getColorScheme() == SP_COLOR_SCHEME_DARK)
            btnBg = btnBg.lighten(2);

        if (state == "active")
            btnBg = btnBg.lighten(-0.15f);
        else if (state == "hover")
            btnBg = btnBg.lighten(0.05f);

        switch (m_buttonStyle) {
        case kButtonFlat: {
            SPColor fill(btnBg.getR(), btnBg.getG(), btnBg.getB(), 210);
            g->fillRoundedRectangle(bounds.x, bounds.y, bounds.width, bounds.height, kModernRadius, fill);
            break;
        }
        case kButtonGradient: {
            SPGradient grad(btnBg.lighten(0.10f), btnBg.lighten(-0.12f), 0);
            g->fillRoundedLinearGradientRectangle(
                bounds.x, bounds.y, bounds.width, bounds.height, kModernRadius, 0, grad);
            break;
        }
        case kButtonModernGradient: {
            float h, s, l;
            btnBg.toHSL(h, s, l);
            SPColor sec = SPColor::fromHSL(h + 60.0f, s * 0.85f, l * 0.85f, btnBg.getA());
            SPGradient grad(btnBg, sec, 0);
            g->fillRoundedLinearGradientRectangle(
                bounds.x, bounds.y, bounds.width, bounds.height, kModernRadius, 0, grad);
            break;
        }
        case kButtonAqua: {
            g->fillRoundedRectangle(bounds.x, bounds.y, bounds.width, bounds.height,
                                    kModernRadius,
                                    SPColor(btnBg.getR(), btnBg.getG(), btnBg.getB(), 230));
            int topH = bounds.height * 2 / 5;
            SPGradient hiSpec(SPColor(255, 255, 255, 165), SPColor(255, 255, 255, 0), 0);
            g->fillRoundedLinearGradientRectangle(
                bounds.x + 3, bounds.y + 2, bounds.width - 6, topH,
                kModernRadius, 0, hiSpec);
            int botH = std::max(bounds.height / 7, 3);
            SPGradient botSpec(SPColor(255, 255, 255, 0), SPColor(255, 255, 255, 45), 0);
            g->fillRoundedLinearGradientRectangle(
                bounds.x + 2, bounds.y + bounds.height - botH - 2,
                bounds.width - 4, botH, 3, 0, botSpec);
            break;
        }
        }

        // Gloss sheen — independent of button style
        if (m_glossy && state != "active")
            drawGlassSheen(g, bounds.x, bounds.y, bounds.width, bounds.height, kModernRadius);

        // Glass rim
        SPColor rim = (elm->getColorScheme() == SP_COLOR_SCHEME_DARK)
            ? SPColor(10, 10, 10, 60)
            : SPColor(200, 200, 200, 100);
        g->fillRoundedRectangle(bounds.x, bounds.y, bounds.width, bounds.height, kModernRadius, rim);
    }

    if (elm->getType() == "appheader") {
        result = true;
        SPColor hdrBg = resolveBackground(elm, tintColor);
        SPColor hdrBase(hdrBg.getR(), hdrBg.getG(), hdrBg.getB(), 230);
        g->fillRectangle(bounds.x, bounds.y, bounds.width, bounds.height, hdrBase);
        if (m_glossy)
            drawGlassSheen(g, bounds.x, bounds.y, bounds.width, bounds.height, 0);
        g->drawLine(bounds.x, bounds.y + bounds.height - 1,
                    bounds.x + bounds.width, bounds.y + bounds.height - 1,
                    sepColor(elm->getColorScheme()));
    }

    if (elm->getType() == "appfooter") {
        result = true;
        SPColor ftrBg = resolveBackground(elm, tintColor);
        SPColor ftrBase(ftrBg.getR(), ftrBg.getG(), ftrBg.getB(), 220);
        g->fillRectangle(bounds.x, bounds.y, bounds.width, bounds.height, ftrBase);
        if (m_glossy)
            drawGlassSheen(g, bounds.x, bounds.y, bounds.width, bounds.height, 0);
        g->drawLine(bounds.x, bounds.y, bounds.x + bounds.width, bounds.y,
                    sepColor(elm->getColorScheme()));
    }

    if (elm->getType() == "treeview") {
        result = true;
        SPColor tvBg = elm->getBackgroundColor();
        if (elm->getColorScheme() == SP_COLOR_SCHEME_DARK)
            tvBg = tvBg.lighten(0.15);
        else
            tvBg = tvBg.lighten(-0.04);

        g->fillRectangle(bounds.x, bounds.y, bounds.width, bounds.height, tvBg);
    }

    if (elm->getType() == "view" || elm->getType() == "viewstack") {
        result = true;
        SPColor vsBg(220, 220, 220, 255);
        if (elm->getColorScheme() == SP_COLOR_SCHEME_DARK) {
            vsBg = SPColor(28, 28, 28, 255); 
            vsBg = vsBg.tint(*tintColor);
        } else { 
            float t = float(tintColor->getA()) / 255.0f * 0.25f;
            vsBg = vsBg.mix(*tintColor, t);
        }
        g->fillRectangle(bounds.x, bounds.y, bounds.width, bounds.height, vsBg);
    }

    return result;
}

void SPModernSkin::drawTableRow(SPGraphicsContext* g, SPTableElement* table, int i, int x, int y, int width, int height, int row)
{
    std::optional<SPColor> tintColor = table->getTintColor();
    SPColor bgColor = resolveBackground(table, tintColor);
    if (table->isRowSelected(row)) {
        // Accent pill for selected row
        SPColor accent = tintColor ? (*tintColor).alpha(1) : SPColor(99, 149, 234, 255);
        SPColor accentFill(accent.getR(), accent.getG(), accent.getB(), 180);
        g->fillRoundedRectangle(x + 2, y + 1, width - 4, height - 2, 6, accentFill);
    } else if (row % 2 == 0) {
        // Very subtle stripe — nearly invisible, just enough to aid scanning
        SPColor stripe = (table->getColorScheme() == SP_COLOR_SCHEME_DARK) ? 
        bgColor.lighten(0.1)
        : bgColor.lighten(-0.1);
        g->fillRectangle(x, y, width, height, stripe);
    } else {
        SPColor stripe = (table->getColorScheme() == SP_COLOR_SCHEME_DARK) ? 
        bgColor.lighten(0.05)
        : bgColor.lighten(-0.05);
        g->fillRectangle(x, y, width, height, stripe);
    }
}

void SPModernSkin::drawTableHeader(SPGraphicsContext* g, SPTableElement* table, int x, int y, int width, int height)
{
    SPColor bgColor = resolveBackground(table, table->getTintColor());
    int colCount = (table->dataSource && table->dataSource->getColumnCount() > 0)
        ? table->dataSource->getColumnCount() : 1;
    int colWidth = width / colCount;

    SPColor hdrBase(bgColor.getR(), bgColor.getG(), bgColor.getB(), 220);
    g->fillRectangle(x, y, width, height, hdrBase);
    if (m_glossy)
        drawGlassSheen(g, x, y, width, height, 0);

    SPColor divider = sepColor(table->getColorScheme());
    for (int col = 1; col < colCount; ++col) {
        int cx = x + col * colWidth;
        g->drawLine(cx, y + 3, cx, y + height - 4, divider);
    }

    // Bottom separator
    g->drawLine(x, y + height - 1, x + width, y + height - 1, divider);
}

void SPModernSkin::drawTabBarTab(SPGraphicsContext* g, SPTabBarElement* tabBar, SPTab* tab, int index, bool active, int x, int y, int width, int height)
{
    SPColor bgColor = resolveBackground(tabBar, tabBar->getTintColor());

    if (active) {
        // Active tab: glass pill
        SPColor activeFill(bgColor.getR(), bgColor.getG(), bgColor.getB(), 200);
        g->fillRoundedRectangle(x + 2, y + 2, width - 4, height - 3, 7, activeFill);

        if (m_glossy) {
            drawGlassSheen(g, x + 2, y + 2, width - 4, height - 3, 7);
        }

        SPColor rim = (tabBar->getColorScheme() == SP_COLOR_SCHEME_DARK)
            ? bgColor.alpha(1)
            : bgColor.alpha(1).lighten(-0.2);
        g->fillRoundedRectangle(x + 2, y + 2, width - 4, height - 3, 7, rim);
    }
    // Inactive tabs: no fill, just spacing — they exist in the empty tab bar background
}

void SPModernSkin::drawTabBar(SPGraphicsContext* g, SPTabBarElement* tabBar, int x, int y, int width, int height)
{
    SPColor bgColor = resolveBackground(tabBar, tabBar->getTintColor());
    SPColor barBase(bgColor.getR(), bgColor.getG(), bgColor.getB(), 215);
    g->fillRectangle(x, y, width, height, barBase);
    if (m_glossy)
        drawGlassSheen(g, x, y, width, height, 0);
    SPColor sep = sepColor(tabBar->getColorScheme());
    //g->drawLine(x, y, x + width, y, sep);
    //g->drawLine(x, y + height - 1, x + width, y + height - 1, sep);
}

void SPModernSkin::drawTreeItem(SPGraphicsContext* g, SPTreeViewElement* treeView, SPTreeItem* item, int itemHeight, int x, int y, int width, int height)
{
    if (item->isSelected() || item->isHighlighted()) {
        short alpha = item->isSelected() ? 160 : 70;
        // Pill-shaped selection indicator inset from edges
        SPColor fill(99, 149, 234, alpha);
        g->fillRoundedRectangle(x + 4, y + 2, treeView->getWidth() - 8, itemHeight - 4, 6, fill);
    }
}

void SPModernSkin::drawSectionHeader(SPGraphicsContext* g, SPTreeViewElement* treeView, const string& title, int x, int y, int width, int height)
{
    // Modern: no background fill — just a muted, small label
    SPFontStyle sectionStyle(
        treeView->getFontStyle().getFamily(),
        treeView->getFontStyle().getHeight() - 2,
        treeView->getFontStyle().getWeight(),
        false, false
    );
    SPColor labelColor(150, 150, 150, 180);
    g->drawString(title, sectionStyle, labelColor, x + 10, y + 3, width - 10, height);
}

SPModernSkin::SPModernSkin() {}
