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
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>
#include "SPXmlParser.h"
#include "SPUriStore.h"

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
    public:
        /// Set by the application layer to receive XUL load-failure notifications.
        static std::function<void(const std::string&)> s_xulErrorHandler;

        /// Mouse capture: when set, this element receives all mousemove/mouseup
        /// events regardless of pointer position (used for drag operations).
        static SPElement* s_capturedElement;
        static void setMouseCapture(SPElement* el) { s_capturedElement = el; }
        static void releaseMouseCapture()          { s_capturedElement = nullptr; }

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
        void request(const string &method, const string &uri,
                     const string &data);
        virtual void onLoad();
        void loadXUL(const string &filename);
        void loadSrc(const string & uri);
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
