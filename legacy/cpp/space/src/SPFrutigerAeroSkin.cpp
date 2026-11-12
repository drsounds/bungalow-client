#include "SPFrutigerAeroSkin.h"

#include "SPGraphicsContext.h"
#include "SPColor.h"
#include "SPElement.h"
#include "SPAppHeaderElement.h"
#include "SPButtonElement.h"
#include "SPTreeViewElement.h"
#include "SPTableElement.h"
#include "SPTableDataSource.h"
#include "libspace.h"

#include <cmath>

using namespace space;

// ── Derived-colour helpers ─────────────────────────────────────────────────────

// ── Sheen helpers ──────────────────────────────────────────────────────────────

// Subtle Aero glass sheen (similar to SPModernSkin but slightly stronger specular)
void SPFrutigerAeroSkin::drawAeroSheen(SPGraphicsContext* g, int x, int y, int w, int h, int r)
{
    int topH = h * 2 / 5;
    int botH = h - topH;
    SPGradient spec(SPColor(255, 255, 255, 110), SPColor(255, 255, 255, 0), 0);
    g->fillRoundedLinearGradientRectangle(x, y, w, topH, r, 0, spec);
    SPGradient refr(SPColor(255, 255, 255, 0), SPColor(255, 255, 255, 32), 0);
    g->fillRoundedLinearGradientRectangle(x, y + topH, w, botH, r, 0, refr);
}

// Heavy WMP10/11-style gloss — taller specular band
void SPFrutigerAeroSkin::drawHeavyGloss(SPGraphicsContext* g, int x, int y, int w, int h, int r)
{
    int topH = h / 2;
    int botH = h - topH;
    SPGradient spec(SPColor(255, 255, 255, 160), SPColor(255, 255, 255, 10), 0);
    g->fillRoundedLinearGradientRectangle(x, y, w, topH, r, 0, spec);
    SPGradient refr(SPColor(255, 255, 255, 0), SPColor(255, 255, 255, 50), 0);
    g->fillRoundedLinearGradientRectangle(x, y + topH, w, botH, r, 0, refr);
}

// ── Header/footer renderer ─────────────────────────────────────────────────────

// Blend the element's tint into a base colour at a fixed low alpha so the
// overall Vista-gray look is preserved while the tint shifts the hue.
static SPColor applyTintToBase(const SPColor& base, std::optional<SPColor> tint)
{
    if (!tint) return base;
    return base.tint(SPColor(tint->getR(), tint->getG(), tint->getB(), 45));
}

void SPFrutigerAeroSkin::drawHeaderFooter(SPGraphicsContext* g, SPElement* elm,
                                           int x, int y, int w, int h, bool /*isHeader*/) const
{
    short scheme = elm->getColorScheme();
    SPColor base  = applyTintToBase(bg(scheme), elm->getTintColor());
    SPColor acc   = accent();

    switch (m_headerStyle) {

    case kAeroHeaderSolid: {
        // Flat fill — just the base background with light translucency
        SPColor fill(base.getR(), base.getG(), base.getB(), 235);
        g->fillRectangle(x, y, w, h, fill);
        break;
    }

    case kAeroHeaderGradient: {
        // WMP 9: gentle top→bottom gradient from slightly lighter to slightly darker
        SPGradient grad(base.lighten(0.12f), base.lighten(-0.08f), 0);
        g->drawLinearGradient({x, y, w, h}, grad);
        break;
    }

    case kAeroHeaderGlossy: {
        // WMP 10: gradient base + Aero glass sheen
        SPGradient grad(base.lighten(0.10f), base.lighten(-0.07f), 0);
        g->drawLinearGradient({x, y, w, h}, grad);
        drawAeroSheen(g, x, y, w, h, 0);
        break;
    }

    case kAeroHeaderAccentGlossy: {
        // WMP 11 beta: strong accent hue dominates, heavy gloss on top
        float accentMix = 0.55f;
        SPColor mixed(
            static_cast<short>(base.getR() * (1.0f - accentMix) + acc.getR() * accentMix),
            static_cast<short>(base.getG() * (1.0f - accentMix) + acc.getG() * accentMix),
            static_cast<short>(base.getB() * (1.0f - accentMix) + acc.getB() * accentMix),
            240
        );
        SPGradient grad(mixed.lighten(0.15f), mixed.lighten(-0.10f), 0);
        g->drawLinearGradient({x, y, w, h}, grad);
        drawHeavyGloss(g, x, y, w, h, 0);
        break;
    }

    case kAeroHeaderDarkGlossy: {
        // WMP 11 final: dark-tinted accent (deep jewel tone) + heavy gloss
        SPColor dark(
            static_cast<short>(acc.getR() * 0.45f),
            static_cast<short>(acc.getG() * 0.45f),
            static_cast<short>(acc.getB() * 0.45f),
            255
        );
        if (scheme == SP_COLOR_SCHEME_DARK) dark = dark.lighten(0.5f);
        SPGradient grad(dark.lighten(0.20f), dark, 0);
        g->drawLinearGradient({x, y, w, h}, grad);
        drawHeavyGloss(g, x, y, w, h, 0);
        break;
    }
    }
}

// ── Separator colour ───────────────────────────────────────────────────────────

static SPColor sepColor(short scheme)
{
    return scheme == SP_COLOR_SCHEME_DARK
        ? SPColor(255, 255, 255, 35)
        : SPColor(0,   0,   0,   22);
}

// ── drawElement ────────────────────────────────────────────────────────────────

bool SPFrutigerAeroSkin::drawElement(SPGraphicsContext* g, SPElement* elm,
                                      const string& state, const sp_rectangle& bounds)
{
    bool result = false;
    short scheme = elm->getColorScheme();

    if (elm->getType() == "input") {
        result = true;
        // Let the platform render the native input; just mark handled.
    } else if (elm->getType() == "splitter") {
        result = true;
        g->fillRectangle(bounds.x, bounds.y, bounds.width, bounds.height,
                         sepColor(scheme));
    } else if (elm->getType() == "button") {
        result = true;

        SPColor base = bg(scheme);
        SPColor acc  = accent();
        static const int kR = 10;

        bool isPrimary = (elm->getStringAttribute("variant", "default") == "primary");
        SPColor btnBg  = isPrimary ? acc : base.lighten(0.06f);

        if (state == "active")
            btnBg = btnBg.lighten(-0.18f);
        else if (state == "hover")
            btnBg = btnBg.lighten(0.08f);

        switch (m_buttonStyle) {
        case kButtonFlat: {
            SPColor fill(btnBg.getR(), btnBg.getG(), btnBg.getB(), 215);
            g->fillRoundedRectangle(bounds.x, bounds.y, bounds.width, bounds.height, kR, fill);
            break;
        }
        case kButtonGradient: {
            SPGradient grad(btnBg.lighten(0.12f), btnBg.lighten(-0.10f), 0);
            g->fillRoundedLinearGradientRectangle(
                bounds.x, bounds.y, bounds.width, bounds.height, kR, 0, grad);
            break;
        }
        case kButtonModernGradient: {
            // Chromatic vertical gradient — accent hue shifts toward +60° for a
            // two-tone look characteristic of contemporary gradient buttons
            float h, s, l;
            btnBg.toHSL(h, s, l);
            SPColor sec = SPColor::fromHSL(h + 60.0f, s * 0.85f, l * 0.85f, btnBg.getA());
            SPGradient grad(btnBg, sec, 0);
            g->fillRoundedLinearGradientRectangle(
                bounds.x, bounds.y, bounds.width, bounds.height, kR, 0, grad);
            break;
        }
        case kButtonAqua: {
            // macOS X Aqua gel: vivid base, wide upper highlight, thin bottom shine
            SPColor aquaBase(btnBg.getR(), btnBg.getG(), btnBg.getB(), 230);
            g->fillRoundedRectangle(bounds.x, bounds.y, bounds.width, bounds.height, kR, aquaBase);
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
        }

        // Gloss sheen — independent overlay, active with any button style
        if (m_glossy && state != "active")
            drawAeroSheen(g, bounds.x, bounds.y, bounds.width, bounds.height, kR);
    } else if (elm->getType() == "appheader") {
        result = true;
        drawHeaderFooter(g, elm, bounds.x, bounds.y, bounds.width, bounds.height, true);
        /*g->drawLine(bounds.x, bounds.y + bounds.height - 1,
                    bounds.x + bounds.width, bounds.y + bounds.height - 1,
                    sepColor(scheme));*/
    } else if (elm->getType() == "appfooter") {
        result = true;
        drawHeaderFooter(g, elm, bounds.x, bounds.y, bounds.width, bounds.height, false);
        /*g->drawLine(bounds.x, bounds.y,
                    bounds.x + bounds.width, bounds.y,
                    sepColor(scheme));*/
    } else if (elm->getType() == "treeview") {
        result = true;
        SPColor tvBg = applyTintToBase(bg(scheme), elm->getTintColor());
        tvBg = (scheme == SP_COLOR_SCHEME_DARK)
            ? tvBg.lighten(0.18f)
            : tvBg.lighten(-0.03f);
        g->fillRectangle(bounds.x, bounds.y, bounds.width, bounds.height, tvBg);
    } else if (elm->getType() == "viewstack") {
        result = true;
        SPColor vsBg = applyTintToBase(bg(scheme), elm->getTintColor());
        g->fillRectangle(bounds.x, bounds.y, bounds.width, bounds.height, vsBg);
    }

    return result;
}

// ── Table ──────────────────────────────────────────────────────────────────────

void SPFrutigerAeroSkin::drawTableRow(SPGraphicsContext* g, SPTableElement* table,
                                       int /*i*/, int x, int y, int width, int height, int row)
{
    short scheme = table->getColorScheme();
    SPColor base = applyTintToBase(bg(scheme), table->getTintColor());

    if (table->isRowSelected(row)) {
        SPColor acc = accent();
        SPColor fill(acc.getR(), acc.getG(), acc.getB(), 185);
        g->fillRoundedRectangle(x + 2, y + 1, width - 4, height - 2, 5, fill);
        if (m_glossy)
            drawAeroSheen(g, x + 2, y + 1, width - 4, height - 2, 5);
    } else {
        float stripe = (row % 2 == 0)
            ? ((scheme == SP_COLOR_SCHEME_DARK) ? 0.12f : -0.10f)
            : ((scheme == SP_COLOR_SCHEME_DARK) ? 0.06f : -0.05f);
        g->fillRectangle(x, y, width, height, base.lighten(stripe));
    }
}

void SPFrutigerAeroSkin::drawTableHeader(SPGraphicsContext* g, SPTableElement* table,
                                          int x, int y, int width, int height)
{
    short scheme = table->getColorScheme();
    SPColor base = applyTintToBase(bg(scheme), table->getTintColor());
    SPGradient grad(base.lighten(0.12f), base.lighten(-0.06f), 0);
    g->drawLinearGradient({x, y, width, height}, grad);
    if (m_glossy)
        drawAeroSheen(g, x, y, width, height, 0);
    g->drawLine(x, y + height - 1, x + width, y + height - 1, sepColor(scheme));
}

// ── Tab bar ────────────────────────────────────────────────────────────────────

void SPFrutigerAeroSkin::drawTabBar(SPGraphicsContext* g, SPTabBarElement* tabBar,
                                     int x, int y, int width, int height)
{
    short scheme = tabBar->getColorScheme();
    SPColor base = applyTintToBase(bg(scheme), tabBar->getTintColor());
    SPColor barFill(base.getR(), base.getG(), base.getB(), 220);
    g->fillRectangle(x, y, width, height, barFill);
    if (m_glossy)
        drawAeroSheen(g, x, y, width, height, 0);
}

void SPFrutigerAeroSkin::drawTabBarTab(SPGraphicsContext* g, SPTabBarElement* tabBar,
                                        SPTab* /*tab*/, int /*index*/, bool active,
                                        int x, int y, int width, int height)
{
    if (!active) return;

    // Tropical gradient pill
    SPGradient tropGrad(tabStart(), tabEnd(), 1);
    g->fillRoundedLinearGradientRectangle(x + 2, y + 2, width - 4, height - 3,
                                          7, 1, tropGrad);
    if (m_glossy)
        drawAeroSheen(g, x + 2, y + 2, width - 4, height - 3, 7);

    // Subtle border
    short scheme = tabBar->getColorScheme();
    SPColor rim = (scheme == SP_COLOR_SCHEME_DARK)
        ? SPColor(255, 255, 255, 50)
        : SPColor(0,   0,   0,   40);
    //g->drawRoundedRectangle(x + 2, y + 2, width - 4, height - 3, 7, rim);
}

// ── Tree view ──────────────────────────────────────────────────────────────────

void SPFrutigerAeroSkin::drawTreeItem(SPGraphicsContext* g, SPTreeViewElement* treeView,
                                       SPTreeItem* item, int itemHeight,
                                       int x, int y, int width, int height)
{
    if (item->isSelected() || item->isHighlighted()) {
        short alpha = item->isSelected() ? 170 : 75;
        SPColor acc = accent();
        SPColor fill(acc.getR(), acc.getG(), acc.getB(), alpha);
        g->fillRoundedRectangle(x + 4, y + 2, treeView->getWidth() - 8,
                                itemHeight - 4, 6, fill);
        if (m_glossy && item->isSelected())
            drawAeroSheen(g, x + 4, y + 2, treeView->getWidth() - 8, itemHeight - 4, 6);
    }
}

void SPFrutigerAeroSkin::drawSectionHeader(SPGraphicsContext* g, SPTreeViewElement* treeView,
                                            const string& title,
                                            int x, int y, int width, int height)
{
    short scheme = treeView->getColorScheme();
    SPColor labelColor = textColor(scheme);
    labelColor = SPColor(labelColor.getR(), labelColor.getG(), labelColor.getB(), 160);

    SPFontStyle sectionStyle(
        treeView->getFontStyle().getFamily(),
        treeView->getFontStyle().getHeight() - 2,
        treeView->getFontStyle().getWeight(),
        false, false
    );
    g->drawString(title, sectionStyle, labelColor, x + 10, y + 3, width - 10, height);
}

// ── Constructor ────────────────────────────────────────────────────────────────

SPFrutigerAeroSkin::SPFrutigerAeroSkin() {}
