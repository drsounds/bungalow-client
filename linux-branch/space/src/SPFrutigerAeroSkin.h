#pragma once

#include "SPSkin.h"
#include "SPColor.h"

namespace space {

/// Header/footer rendering modes — inspired by the Windows Media Player lineage.
enum AeroHeaderStyle {
    kAeroHeaderSolid       = 0,  ///< Flat fill, like SPModernSkin
    kAeroHeaderGradient    = 1,  ///< Gentle top-to-bottom gradient (WMP 9)
    kAeroHeaderGlossy      = 2,  ///< Gradient + glass sheen (WMP 10)
    kAeroHeaderAccentGlossy= 3,  ///< Strong accent hue + heavy gloss (WMP 11 beta)
    kAeroHeaderDarkGlossy  = 4   ///< Dark-tinted accent + gloss (WMP 11 final)
};

/// Frutiger Aero skin — Vista-era translucent aqua aesthetic.
///
/// All colours are derived from three parameters:
///   m_hue        0–360   primary hue (shifts the whole palette)
///   m_saturation 0–1     how vivid the tints are
///   m_lightness  0–1     overall brightness offset
///
/// Light mode:  pale tinted-gray background, tropical gradient tabs, dark-blue text.
/// Dark mode:   same hue but dark navy background, lighter tropical tabs.
class SPFrutigerAeroSkin : public SPSkin {
public:
    SPFrutigerAeroSkin();

    // ── Colour parameters ─────────────────────────────────────────────────────
    void  setHue(float h)        { m_hue = h; }
    float getHue()  const        { return m_hue; }

    void  setSaturation(float s) { m_saturation = s; }
    float getSaturation() const  { return m_saturation; }

    void  setLightness(float l)  { m_lightness = l; }
    float getLightness() const   { return m_lightness; }

    void           setHeaderStyle(AeroHeaderStyle s) { m_headerStyle = s; }
    AeroHeaderStyle getHeaderStyle() const           { return m_headerStyle; }

    // ── SPSkin interface ──────────────────────────────────────────────────────
    void setGlossy(bool v)  override { m_glossy = v; }
    bool isGlossy()   const override { return m_glossy; }
    void setButtonStyle(SPButtonStyle s) override { m_buttonStyle = s; }
    SPButtonStyle getButtonStyle() const override { return m_buttonStyle; }

    bool drawElement(SPGraphicsContext* gc, SPElement* elm,
                     const string& state, const sp_rectangle& bounds) override;

    void drawTableRow(SPGraphicsContext* g, SPTableElement* table,
                      int i, int x, int y, int width, int height, int row) override;
    void drawTableHeader(SPGraphicsContext* g, SPTableElement* table,
                         int x, int y, int width, int height) override;

    void drawTabBarTab(SPGraphicsContext* g, SPTabBarElement* tabBar, SPTab* tab,
                       int index, bool active,
                       int x, int y, int width, int height) override;
    void drawTabBar(SPGraphicsContext* g, SPTabBarElement* tabBar,
                    int x, int y, int width, int height) override;

    void drawTreeItem(SPGraphicsContext* g, SPTreeViewElement* treeView,
                      SPTreeItem* item, int itemHeight,
                      int x, int y, int width, int height) override;
    void drawSectionHeader(SPGraphicsContext* g, SPTreeViewElement* treeView,
                           const string& title,
                           int x, int y, int width, int height) override;

private:
    float           m_hue         = 200.0f;
    float           m_saturation  = 0.70f;
    float           m_lightness   = 0.50f;
    AeroHeaderStyle m_headerStyle = kAeroHeaderGlossy;
    bool            m_glossy      = true;
    SPButtonStyle   m_buttonStyle = kButtonAqua;

    // ── Derived-colour helpers ────────────────────────────────────────────────

    // ── Drawing helpers ───────────────────────────────────────────────────────
    void drawHeaderFooter(SPGraphicsContext* g, SPElement* elm,
                          int x, int y, int w, int h, bool isHeader) const;
    static void drawAeroSheen(SPGraphicsContext* g, int x, int y, int w, int h, int r);
    static void drawHeavyGloss(SPGraphicsContext* g, int x, int y, int w, int h, int r);
};

} // namespace space
