#ifndef ELEMENT_H
#define ELEMENT_H

#define SP_COLOR_SCHEME_DEFAULT 0
#define SP_COLOR_SCHEME_DARK 1
#define SP_COLOR_SCHEME_LIGHT 2

#define SP_STATE_NORMAL 0
#define SP_STATE_HOVER 1
#define SP_STATE_ACTIVE 2

#include "SPNode.h"
#include "SPFontStyle.h"
#include <functional>
#include <memory>
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>
#include "SPXmlParser.h"
#include "SPUriStore.h"
#include "SPScrollBar.h"

using namespace std;

enum sp_size_scale : short {
    undefined = 0x0,
    pixel = 0x1,
    percent = 0x2
};

typedef float sp_value;

namespace space {
    struct sp_unit_value {
        sp_value value;
        sp_size_scale unit;
    };
    class Space;
    class SPMessage;
    class SPSkin;
    class SPGraphicsContext;
    class SPStylesheet;
    class SPWindowElement;
    class SPImage;
    class SPFontStyle;
    class SPMouseEventArgs;
    class SPRichText;

    /// What an element does with content taller than itself.
    enum class SPOverflow {
        Hidden,  ///< clipped (the default)
        Auto     ///< scrolls with the mouse wheel and a skinned scrollbar
    };

    class SPElement : public SPNode {
    protected:
        SPImage *m_backgroundImage;
        vector<string> m_classList;
        SPSkin *m_skin = nullptr;
        bool scrollable;
        bool visible = true;
        short m_colorScheme = 0;
    	int m_state = 0;
        sp_margin margins;
        sp_margin padding;
        SPFontStyle fontStyle;
        string id;
        string data;
        int m_width;
        int m_height;
        int x;
        int y;
        int z;
        int scrollX;
        int scrollY;
        sp_unit_value m_unitWidth = { 0, undefined };
        sp_unit_value m_unitHeight = { 0, undefined };
        sp_rectangle absoluteBounds;
    	int m_flex = 0;
        bool clipView = true;
        std::optional<SPGradient> m_gradient;
        bool m_hasDefaultGradient = false;
        std::optional<SPColor> m_tintColor;
        std::optional<SPColor> m_activeColor;
        std::optional<SPColor> m_backgroundColor;
        std::optional<SPColor> m_foregroundColor;
    
        bool drawBorder = false;
    	SPElement *m_parsingElement;
    	int m_parsingDepth = 0;
        int m_gap = 0;
        /// Re-renders the src template when its URI entity changes.
        SPUriStore::Subscription m_srcSubscription;
        /// Keeps the inner text in sync with the "entity" attribute's URI.
        SPUriStore::Subscription m_entitySubscription;
        void renderTemplate(const string &source, const string &method, const string &uri,
                            const string &action, const nlohmann::json &requestData);
        void bindEntity(const string &uri);
        void applyEntity(const SPUriEntry &entry);

        SPOverflow m_overflowY = SPOverflow::Hidden;
        /// The scrollbar part held down with the mouse, if any.
        std::optional<SPScrollBarPart> m_scrollBarPressed;
        /// Distance from the thumb's top to the pointer while dragging it.
        int m_scrollThumbGrab = 0;
        /// Pixels the scrollbar buttons scroll by.
        static constexpr int kScrollLineStep = 24;

        /// Where the vertical scrollbar's parts go for the current bounds and scroll position.
        SPScrollBarLayout getVerticalScrollBarLayout();
        void paintVerticalScrollBar(SPGraphicsContext *c);
        /// Handles a mouse event on the scrollbar. Returns true if it was
        /// consumed, so it is neither handled by this element nor its children.
        bool handleScrollBarMouse(const string &event, SPMouseEventArgs *e);
        /// Called after the vertical scroll position changed.
        virtual void onScrolled() {}

        // ── Rich text and links (see SPRichText) ─────────────────────────
        /// The markup set with setInnerHTML(), while it is still the text.
        std::shared_ptr<SPRichText> m_richText;
        /// The inner text setInnerHTML() left; a later setInnerText() makes
        /// the element plain again.
        string m_richPlain;
        /// Where the text was last painted, in window coordinates.
        sp_position m_richTextOrigin { 0, 0 };
        /// The link under the pointer, "" when there is none.
        string m_hoverLinkUri;
        /// The element whose link the pointer is over, and whether it said
        /// so again during the mouse move being dispatched.
        static SPElement* s_linkHoverElement;
        static bool s_linkHoverClaimed;
        /// The label to draw instead of the plain inner text, or nullptr.
        virtual SPRichText *getRichText();
        /// Called on a mouse move over a link of this element: shows the hand
        /// cursor and marks the link as hovered until the pointer leaves it.
        void claimLinkHover(const string &uri);
        /// The pointer left this element's link.
        virtual void clearLinkHover();
        /// Follows a link of the element's own text on a click, and tracks
        /// the one under the pointer. Returns true if the event was consumed.
        bool handleRichTextMouse(const string &event, SPMouseEventArgs *e);
    public:
        /// Sets the element's text as markup: links, bold, italic, underline,
        /// line breaks and images; see SPRichText for the tags. Text that
        /// comes from data must be escaped (SPRichText::escape). XUL sets
        /// this by itself for a body that mixes text and those tags:
        ///   <text>By <link uri="bungalow:artist:1">Queen</link></text>
        void setInnerHTML(const string &markup);
        /// The markup set with setInnerHTML(), or the inner text, escaped.
        string getInnerHTML();
        /// The color links are drawn in: the "linkcolor" attribute, or a
        /// blue that suits the color scheme.
        SPColor getLinkColor();
        /// Follows a link: asks the window to navigate to uri. The
        /// application decides what that means for the URI (a view, a site,
        /// the system browser).
        virtual void activateLink(const string &uri);
        /// Bracket the dispatch of one mouse move, so that the hovered link
        /// is forgotten once the pointer is over no link at all.
        static void beginLinkHoverPass();
        static void endLinkHoverPass();


        /// overflow="auto" | "hidden"; see SPOverflow.
        SPOverflow getOverflowY() const { return m_overflowY; }
        void setOverflowY(SPOverflow value) { m_overflowY = value; }
        /// Height of the content, measured from the element's top. By default
        /// the bottom of the lowest visible child plus the bottom padding.
        virtual int getContentHeight();
        /// How far the content can scroll vertically; 0 when it fits.
        int getMaxScrollY();
        /// True when overflow is auto and the content is taller than the element.
        bool hasVerticalScrollBar();
        /// The skin's scrollbar size for this element.
        SPScrollBarMetrics getScrollBarMetrics();
        /// Width the vertical scrollbar takes up, or 0 if it is not shown.
        int getVerticalScrollBarWidth();
        /// Scrolls to y, clamped to the content, and repaints.
        void scrollToY(int y);
        /// Scrolls the innermost scrollable element under the pointer; when it
        /// is already at its end the wheel scrolls its ancestors instead.
        /// Returns true if anything scrolled.
        virtual bool mouseWheel(SPMouseEventArgs *e);

        /// Set by the application layer to receive XUL load-failure notifications.
        static std::function<void(const std::string&)> s_xulErrorHandler;

        /// Mouse capture: when set, this element receives all mousemove/mouseup
        /// events regardless of pointer position (used for drag operations).
        static SPElement* s_capturedElement;
        static void setMouseCapture(SPElement* el) { s_capturedElement = el; }
        static void releaseMouseCapture()          { s_capturedElement = nullptr; }

        /// Cursor hint for the platform layer: elements set this while the
        /// pointer hovers (or drags) them so the OS cursor can be swapped,
        /// e.g. to a resize arrow over a splitter. Reset to Arrow before each
        /// mousemove dispatch.
        enum class SPCursorHint { Arrow, ResizeHorizontal, Hand };
        static SPCursorHint s_cursorHint;
        static void setCursorHint(SPCursorHint hint) { s_cursorHint = hint; }
        static SPCursorHint getCursorHint()          { return s_cursorHint; }

    	void setState(int value) {
    		this->m_state = value;
    		this->invalidate();
    	}
        int getGap() const {
            return m_gap;
        }
        void setGap(int value) {
            m_gap = value;
        }
        nlohmann::json getFormData();
    	int getState() const {
    		return this->m_state;
    	}
    	virtual SPElement *getSlotElement();
    	int getFlex() const {
    		return m_flex;
    	}
    	void setFlex(int value) {
    		this->m_flex = value;
    	}

        void setColorAttribute(const string& attrName, SPColor value) override;
        SPColor getColorAttribute(const string& title, SPColor defaultValue) override;

		SPElement *getDeepestElement(int level = 0);

    	SPWindowElement *getWindowElement();
    	string getSrc();
        void refresh();
        void setSrc(const string& value);
        SPElement *getParentElement()
        {
            return static_cast<SPElement *>(this->getParentNode());
        }
		sp_rectangle getAbsoluteBounds() const {
	        return this->absoluteBounds;
        }
    	void setAbsoluteBounds(sp_rectangle value) {
	        this->absoluteBounds = value;
        }
        string getId() const;
        void setId(const string& value) override;
        string getInnerXUL();
        short getColorScheme();
        void setColorScheme(short value);
        std::optional<SPGradient> getGradient() const {
            return this->m_gradient;
        }
        void setGradient(const SPGradient& value) {
            this->m_gradient = value;
        }
        std::optional<SPColor> getActiveColor();
        void setActiveColor(const SPColor& value);
        std::optional<SPColor> getTintColor();
        void setTintColor(const std::optional<SPColor>& value);
        string getId();
        int notify(const string &event, SPObject *sender,
                   SPEventArgs *data) override;
        bool hasDefaultGradient() const
        {
            return m_hasDefaultGradient;
        }
        void setHasDefaultGradient(bool value)
        {
            m_hasDefaultGradient = value;
        }
        SPSkin *getSkin();

        sp_unit_value getUnitWidth() const {
            return m_unitWidth;
        }
        sp_unit_value getUnitHeight() const {
            return m_unitHeight;
        }
        void setUnitWidth(const sp_unit_value& value) {
            m_unitWidth = value;
        }
        void setUnitHeight(const sp_unit_value& value) {
            m_unitHeight = value;
        }
        void increaseParsingDepth();
    	void decreaseParsingDepth();
    	int getParsingDepth() const {
    		return this->m_parsingDepth;
    	}
    	SPElement *getParsingElement() const {
    		return this->m_parsingElement;
    	}
    	void setParsingElement(SPElement *value) {
    		this->m_parsingElement = value;
    	}

        void setSkipElement(bool value) {
    		this->m_skipElement = value;
    	}

        void postBack(const nlohmann::json& data);

        bool m_skipElement = false;

        bool getSkipElement() const {
    		return this->m_skipElement;
    	}
        SPElement();
        void setSkin(SPSkin *value);
        virtual void resize(int width, int height);
        virtual void scroll(int x, int y);
        void scrollTo(int x, int y);
    	void applySkin();
    	void notifyThemeChanged();
        virtual void scroll(int scrollX, int scrollY, int x, int y);
        void applyColorAttributeFromSkin (string attr, string skin);
        void applyImageAttributeFromSkin (string attr, string skin);
        void generateAndSetDefaultGradient();
        SPGradient generateDefaultGradient();
        bool hasClass(string _class);
        void addClass(string _class);
        void removeClass(string _class);
        void applyStylesheet(SPStylesheet *style);
    	SPColor getBackgroundColor();
    	void setBorderColor(const SPColor& value)
    	{
    		this->setColorAttribute("border-color", value);
    	}
    	SPColor getBorderColor() {
    		return this->getColorAttribute("border-color", SPColor(0, 0, 0, 0));
    	}
        void setBackgroundColor(const SPColor& value)
        {
            this->setColorAttribute("bgcolor", value);
        }
        SPColor getForegroundColor();
        void setForegroundColor(const SPColor& value)
        {
            this->setColorAttribute("fgcolor", value);
        }

        virtual void paint(int x, int y, SPGraphicsContext *c);
        string getType() override;
        virtual void setUri(const string &uri) override;
        virtual void paint(int x, int y, int width, int height, SPGraphicsContext* c);
        void resizeToText(bool width, bool height);
        sp_rectangle getTextBounds();

        void setBackgroundImage(SPImage *value);
        void invalidate();
        bool isVisible() const
        {
            return this->visible;
        }
        virtual SPGraphicsContext *createGraphics();
        virtual void show();
        virtual void hide();

        virtual void setVisible(bool value) {
            this->visible = value;
           // ((Element *)this->getParentNode())->pack();
        }
        bool isScrollable() const
        {
            return this->scrollable;
        }
        void setScrollable(const bool value) {
            this->scrollable = value;
        }
        int getScrollX() const
        {
            return this->scrollX;
        }
        int getScrollY() const
        {
            return this->scrollY;
        }
        void setScrollX(int value) {
            this->scrollX = value;
        }
        void setScrollY(int value) {
            this->scrollY = value;
        }
        SPFontStyle getFontStyle();
        void setFontStyle(const SPFontStyle& value)  {
            this->fontStyle = value;
        }

        explicit SPElement(SPElement *parent);
        virtual void request(const string &method, const string &uri,
                             const string &data);
        virtual void onLoad();
        void loadXUL(const string &filename);
        /// Called when the src attribute changes: renders the template for
        /// the URI into the element. Images load their picture instead.
        virtual void loadSrc(const string & uri);
        void onAttributeChanged(SPObject *sender, SPEventArgs *e);

        ~SPElement() override;
        int getX();
        void processEvent(const SPEvent& event);
        virtual void processMessage(const SPMessage&);
        sp_margin getMargins();
        sp_margin getPadding();
        void setPadding(sp_margin value);
    	void setMargins(sp_margin value);
        virtual SPElement *createElement(const string& tagName, SPElement* parent);
        SPElement *getElementById(const string &id) const;
        SPElement *parseElement(const SPXmlNode& xmlNode, SPElement *parent,
                                SPElement *document);
        void onPostBackButtonClick(SPObject* sender, SPEventArgs* event);
        void endParseElement(void *data, const char *el);
        void startParseElement(void *data, const char *el, const char **attr);
        void setInnerXUL(const string &value);
        void loadXULFromXMLDoc(const SPXmlNode& xmlDoc);
        SPElement *getTabBarViewElement();
        virtual SPElement *getViewStackElement();
        virtual void mouseMove(int, int) {}
        virtual void mouseClick(int, int, int);
        virtual void mouseDown(SPObject* sender, SPEventArgs* e);
        virtual void onClick(int, int, int);
        virtual void mouseUp(SPObject* sender, SPEventArgs* e);
        int getY(); 
        int getWidth();
        int getHeight();
        int getZ();
        void setX(int x);
        void setY(int y);
        void setWidth(int width);
        void setHeight(int height);
        void setZ(int z);
        virtual void pack();

    };
}

#endif // ELEMENT_H
