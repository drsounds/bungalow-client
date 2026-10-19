#ifndef SP_XML_SKIN_H
#define SP_XML_SKIN_H

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "SPColor.h"
#include "SPModernSkin.h"
#include "libspace.h"

namespace space {
    class SPImage;
    struct SPXmlNode;

    /// A skin described by a skin.xml stylesheet inside an app bundle
    /// (<app>/skins/<name>/skin.xml).
    ///
    /// <resources> declares sprite images (<image src="sp://app/..." id="skin"/>,
    /// optionally brightness="0.4" for a darker variant of the same sheet) and
    /// named colors with per-mode states. Each <selector match="sp-button"
    /// state="active" mode="dark"> holds <property> entries:
    ///   name="background" value="$color" | "#rrggbb"     → solid fill
    ///   name="skin"/"background" value="url(#imageId)"
    ///        slice="x1,y1;x2,y2" nine-patch="l,t,r,b"   → nine-patch sprite
    ///   name="padding" value="l,t,r,b"                  → content inset (getContentBounds)
    ///   name="color" value="$color.foreground" | "#hex" → label colour (buttons, checkboxes)
    ///   name="radius" value="6"                         → rounds solid backgrounds
    /// A selector with mode="dark"/"light" wins over one without a mode in
    /// that colour scheme and is ignored in the other.
    ///
    /// Sprites and colors are colorized by the skin's hue/saturation/lightness
    /// so the HSL sliders tint the whole skin. Anything the stylesheet does not
    /// style falls back to SPModernSkin's procedural drawing.
    class SPXmlSkin : public SPModernSkin {
    public:
        SPXmlSkin();
        ~SPXmlSkin() override;

        /// Loads the stylesheet. Relative image paths resolve against baseDir.
        bool load(const std::string& skinXmlPath, const std::string& baseDir);

        const std::string& getName() const { return m_name; }

        /// Resolves "$name", "$name.foreground" or "#hex" for the given colour
        /// scheme, tinted by the current HSL.
        std::optional<SPColor> resolveColor(const std::string& value, short scheme, bool foreground = false) const;

        bool drawElement(SPGraphicsContext* gc, SPElement* elm, const string& state, const sp_rectangle& bounds) override;
        /// Insets bounds by the matched selector's padding="l,t,r,b" property.
        sp_rectangle getContentBounds(SPElement* elm, const sp_rectangle& bounds) override;
        /// The matched selector's color property.
        std::optional<SPColor> getTextColor(SPElement* elm, const string& state) override;
        void drawTabBar(SPGraphicsContext* g, SPTabBarElement* tabBar, int x, int y, int width, int height) override;
        /// sp-table-row with state "selected", "odd" or "even".
        void drawTableRow(SPGraphicsContext* g, SPTableElement* table, int i, int x, int y, int width, int height, int row) override;
        /// sp-table-header.
        void drawTableHeader(SPGraphicsContext* g, SPTableElement* table, int x, int y, int width, int height) override;
        /// sp-treeitem with state "selected" or "hover".
        void drawTreeItem(SPGraphicsContext* g, SPTreeViewElement* treeView, SPTreeItem* item, int itemHeight, int x, int y, int width, int height) override;
        void drawTabBarTab(SPGraphicsContext* g, SPTabBarElement* tabBar, SPTab* tab, int index, bool active, int x, int y, int width, int height) override;

    private:
        struct ColorState {
            std::optional<SPColor> background;
            std::optional<SPColor> foreground;
        };
        struct NamedColor {
            std::map<std::string, ColorState> modes;  ///< "dark" / "light"
            bool tint = true;
            float tintSaturation = 0.12f;
        };
        struct ImageResource {
            std::shared_ptr<SPImage> image;  ///< shared by resources with the same src
            bool tint = true;
            float tintSaturation = 0.35f;
            float brightness = 1.0f;         ///< multiplies sprite lightness
        };
        struct Property {
            std::string name;
            std::string value;
            std::string imageId;                 ///< set for url(#id) values
            int sx = 0, sy = 0, sw = 0, sh = 0;  ///< sprite rect within the image
            int left = 0, top = 0, right = 0, bottom = 0;
            std::unique_ptr<SPImage> source;     ///< untinted crop of the sprite
            std::unique_ptr<SPImage> tinted;     ///< source colorized by the current HSL
        };
        struct Rule {
            std::string match;
            std::string state;
            std::string mode;  ///< "dark", "light" or "" for both
            std::vector<std::unique_ptr<Property>> properties;
        };

        void parseResources(const SPXmlNode& node, const std::string& baseDir);
        void parseSelector(const SPXmlNode& node);
        void refreshTint();
        SPColor tintColor(const SPColor& color, float saturationFloor, float brightness = 1.0f) const;
        const Rule* findRule(const std::vector<std::string>& selectors,
                             const std::vector<std::string>& states, short scheme, bool& matched) const;
        const Rule* findRule(SPElement* elm, const string& state, bool& matched) const;
        std::vector<std::string> selectorsFor(SPElement* elm) const;
        void drawRule(SPGraphicsContext* g, const Rule& rule, short scheme, const sp_rectangle& bounds);
        void drawLabel(SPGraphicsContext* g, SPElement* elm, const Rule& rule, const sp_rectangle& bounds) const;
        void drawNinePatch(SPGraphicsContext* g, const Property& p, const sp_rectangle& bounds) const;

        std::string m_name;
        std::map<std::string, NamedColor> m_colors;
        std::map<std::string, ImageResource> m_images;
        std::vector<std::unique_ptr<Rule>> m_rules;

        // HSL the cached tinted sprites were generated for.
        float m_tintedHue = -1, m_tintedSaturation = -1, m_tintedLightness = -1;
    };
}

#endif
