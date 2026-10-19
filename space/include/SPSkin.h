#ifndef SKIN_H
#define SKIN_H
#include <map>
#include <optional>
#include <string>

#include "SPColor.h"

#include "SPTabBarElement.h"
#include "SPTableElement.h"
#include "SPTreeItem.h"
#include "SPTreeViewElement.h"

#define SP_MODE_DEFAULT 0
#define SP_MODE_DARK 1
#define SP_MODE_LIGHT 2

namespace space {
/// Button texture directive — skins use this to decide how to paint button fills.
/// Glossy adds a separate specular sheen layer on top, independent of this setting.
enum SPButtonStyle {
    kButtonFlat           = 0,  ///< Plain solid fill, no gradient
    kButtonGradient       = 1,  ///< Classic Bootstrap 1.x vertical light→dark gradient
    kButtonModernGradient = 2,  ///< Chromatic vertical gradient (accent → secondary hue)
    kButtonAqua           = 3   ///< macOS X Aqua gel — vivid base + white highlight band
};
}


namespace space
{
    class SPElement;
    class SPGraphicsContext;
}

using namespace std;
namespace space {
    struct sp_rectangle;
    class SPImage;
class SPColor;
class pixel;
class slice {
public:
    SPImage *image;
    int x1;
    int y1;
    int x2;
    int y2;
    int leftBorder;
    int topBorder;
    int rightBorder;
    int bottomBorder;
    slice(SPImage *image, int x1, int x2, int y1, int y2, int leftBorder, int topBorder, int rightBorder, int bottomBorder);
};
class SPSkin
{
    private:
        float m_hue = 0;
        float m_saturation = 0;
        float m_lightness = 0;
    public:
        void  setHue(float h)        { m_hue = h; }
        float getHue()  const        { return m_hue; }

        void  setSaturation(float s) { m_saturation = s; }
        float getSaturation() const  { return m_saturation; }

        void  setLightness(float l)  { m_lightness = l; }
        float getLightness() const   { return m_lightness; }

        /// Very pale tinted background (light mode).
        SPColor bgLight()    const;
        /// Dark navy background (dark mode).
        SPColor bgDark()     const;
        /// Resolved background for the given scheme.
        SPColor bg(short scheme) const { return scheme == SP_COLOR_SCHEME_DARK ? bgDark() : bgLight(); }

        /// Vivid accent — used for selections, active tabs, primary buttons.
        SPColor accent()     const;
        /// Tropical tab-gradient start.
        SPColor tabStart()   const;
        /// Tropical tab-gradient end (shifted +30° hue).
        SPColor tabEnd()     const;

        /// Text colour appropriate for the colour scheme.
        SPColor textColor(short scheme) const;

        map<string, SPImage *> images;
        map<string, pixel *> colors;
        map<string, slice *> slices;
        SPImage *image;
        SPSkin();
        SPSkin(const string& bitmap);
        virtual ~SPSkin();
        bool hasColor(string selector);
        bool hasImage(string selector);
        SPColor getColor(string selector, string def);
        SPImage *getImage(string selector, string def);
        virtual bool drawElement(SPGraphicsContext *gc, SPElement *, const string& state, const sp_rectangle& bounds) { return false; }
        /// The part of an element's bounds left for its content once the
        /// skin's frame is drawn, e.g. where a native edit control goes.
        virtual sp_rectangle getContentBounds(SPElement *, const sp_rectangle& bounds) { return bounds; }
        /// Label colour the skin wants for an element in a state, if any.
        virtual std::optional<SPColor> getTextColor(SPElement *, const string& state) { return std::nullopt; }
        virtual void drawTableRow(SPGraphicsContext* g, SPTableElement* table, int i, int x, int y, int width, int height, int row) {}
        virtual void drawTableHeader(SPGraphicsContext* g, SPTableElement* table, int x, int y, int width, int height) {}
        virtual void drawTabBarTab(SPGraphicsContext* g, SPTabBarElement *tabBar, SPTab *tab, int index, bool active, int x, int y, int width, int height) {}
        virtual void drawTabBarToolButton(SPGraphicsContext* g, SPTabBarElement* tabBar, SPTabToolButton* btn, int x, int y, int width, int height) {}
        virtual void drawTabBar(SPGraphicsContext* g, SPTabBarElement* tabBar, int x, int y, int width, int height) {}
        virtual void drawTreeItem(SPGraphicsContext* g, SPTreeViewElement *treeView, SPTreeItem* item, int itemHeight, int x, int y, int width, int height) {}
        virtual void drawSectionHeader(SPGraphicsContext* g, SPTreeViewElement* treeView, const string& title, int x, int y, int width, int height) {}

        virtual void setGlossy(bool value) {}
        virtual bool isGlossy() const { return true; }

        virtual void setButtonStyle(SPButtonStyle s) {}
        virtual SPButtonStyle getButtonStyle() const { return kButtonGradient; }
};
}
#endif // SKIN_H
