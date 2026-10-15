#include "SPElement.h"

#include "SPButtonElement.h"

#include <iostream>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <utility>
#include "Space.h"

#include "SPWindowElement.h"
#include "SPObservable.h"
#include "SPMouseEventArgs.h"
#include "SPGraphicsContext.h"
#include "SPHBoxElement.h"
#include "SPSkin.h"
#include "SPSplitterElement.h"
#include "SPImage.h"
#include "SPStylesheet.h"
#include "SPFontStyle.h"
#include "SPImageElement.h"
#include "SPInputElement.h"
#include "SPSectionElement.h"
#include "SPViewStackElement.h"
#include "SPTabBarViewElement.h"
#include "SPTextElement.h"
#include "SPViewElement.h"
#include "SPTemplate.h"

#include <nlohmann/json.hpp>
#include <ctime>
#include <exception>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace space {
	std::function<void(const std::string&)> SPElement::s_xulErrorHandler;
	SPElement* SPElement::s_capturedElement = nullptr;

	std::optional<SPColor> SPElement::getActiveColor()
	{
		if (auto activeColor = this->getTintColor(); activeColor.has_value())
			return activeColor;

		std::optional<SPColor> ret;
		if (this->getWindowElement() != nullptr && this->getType() != "window")
			ret = this->getWindowElement()->getActiveColor();
		if (this->getParentElement() != nullptr && this->getType() != "window")
			ret = this->getParentElement()->getActiveColor();
		return ret;
	}

	void SPElement::setActiveColor(const SPColor& value)
	{
		this->m_activeColor = value;
	}

	std::optional<SPColor> SPElement::getTintColor()
	{
		if (m_tintColor.has_value())
			return m_tintColor;

		std::optional<SPColor> ret;
		if (this->getWindowElement() != nullptr && this->getType() != "window")
			ret = this->getWindowElement()->getTintColor();
		if (this->getParentElement() != nullptr && this->getType() != "window")
			ret = this->getParentElement()->getTintColor();
		return ret;
	}

	void SPElement::setTintColor(const std::optional<SPColor>& value)
	{
		this->m_tintColor = value;
	}

	string SPElement::getId() {
		return this->m_id;
	}

	int SPElement::notify(
		const string &event,
		SPObject *sender,
	    SPEventArgs *data
	) {
		if (event == "load") {
			cout << "Load";
		}
		int ret = SPNode::notify(event, sender, data);
		for (auto *child: this->children) {
			if (data == nullptr) {
				continue;
			}
			if (data->getType() == "SPMouseEventArgs") {
				auto *mouseEventArgs = dynamic_cast<SPMouseEventArgs *>(data);
				auto element = dynamic_cast<SPElement *>(child);
				if (child != nullptr) {
					if (!element->isVisible()) continue;
					bool captured = (s_capturedElement != nullptr && element == s_capturedElement);
					bool inBounds =
						mouseEventArgs->getX() > element->getAbsoluteBounds().x &&
						mouseEventArgs->getX() < element->getAbsoluteBounds().x + element->getAbsoluteBounds().width &&
						mouseEventArgs->getY() > element->getAbsoluteBounds().y &&
						mouseEventArgs->getY() < element->getAbsoluteBounds().y + element->getAbsoluteBounds().height;
					if (captured || inBounds)
					{
						if (event == "mousemove") {
							element->notify(event, sender, data);
							ret = 1;
						}
						if (event == "mouseup") {
							element->setState(SP_STATE_NORMAL);
							element->notify(event, sender, data);
							ret = 1;
							continue;
						}
						if (event == "mousedown")
						{
							if (element->getType() == "tabbar")
							{
								// cout << "Element: " << this->getType() << "#" << this->getId() << " notified child: " << element->getType() << "#" << element->getId() << " event: " << event << endl;
							}
							element->setState(SP_STATE_ACTIVE);
							element->notify(event, sender, data);
							ret = 1;
						}
						if (event == "click")
						{
							element->setState(SP_STATE_NORMAL);
							ret = 1;
							element->notify(event, element, data);

						}
						if (event == "load")
						{
							ret = 1;
							element->notify(event, element, data);

						}
						// cout << "Element: " << this->getType() << "#" << this->getId() << " notified child: " << element->getType() << "#" << element->getId() << " event: " << event << endl;
					}
				}
			}
		}
		return ret;
	}

	SPSkin *SPElement::getSkin()
	{
		if (this->m_skin)
		{
			return this->m_skin;
		}
		if (this->getParentElement() != nullptr) {
			SPSkin *skin = this->getParentElement()->getSkin();
			if (skin != nullptr) {
				return skin;
			}
		}
		if (this->getWindowElement() != nullptr)
		{
			return this->getWindowElement()->getSkin();
		}
		return nullptr;
	}

	void SPElement::decreaseParsingDepth() {
		this->m_parsingDepth--;
	}


	void SPElement::increaseParsingDepth() {
		this->m_parsingDepth++;
	}

	void SPElement::postBack(const nlohmann::json& payload) {
		string uri = this->getSrc();
		this->request("POST", uri, payload.dump());
	}

	SPElement::SPElement() : SPNode(), fontStyle(SPFontStyle("Tahoma", 11, 1, false, false)) {
		this->padding = { 0, 0, 0, 0 };
		this->margins = { 0, 0, 0, 0 };
	}

	SPGraphicsContext* SPElement::createGraphics()
	{
		return this->getSpace()->getGraphicsContext();
	}

	void SPElement::show() {
	    this->setVisible(true);
	}
	void SPElement::hide() {
	    this->setVisible(false);
	}
	void SPElement::mouseClick(int mouseButton, int x, int y) {

	}

	void SPElement::setSkin(SPSkin *value) {
		this->m_skin = value;
		this->applySkin();
	}

	void SPElement::onClick(int mouseButton, int x, int y) {
		int xx =  x - this->getAbsoluteBounds().y;
	    int yy = y - this->getAbsoluteBounds().y;
		SPMouseEventArgs *me = new SPMouseEventArgs(mouseButton, xx, yy);

		this->setState(SP_STATE_NORMAL);
		this->notify(string("click"), this, me);
		for(vector<SPNode *>::iterator it = this->getChildNodes().begin(); it != this->getChildNodes().end(); ++it) {
			SPElement *elm = static_cast<SPElement *>(*it);

			if(!elm->isVisible()) continue;

			if(x > elm->getAbsoluteBounds().x && x < elm->getAbsoluteBounds().x + elm->getAbsoluteBounds().width &&
				y > elm->getAbsoluteBounds().y && y < elm->getAbsoluteBounds().y + elm->getAbsoluteBounds().height) {

				elm->onClick(mouseButton, x , y);
			}
		}
	}


	void SPElement::mouseDown(SPObject* sender, SPEventArgs* e) {
		SPMouseEventArgs *me = dynamic_cast<SPMouseEventArgs *>(e);
		int x = me->getX();
		int y = me->getY();
		int xx = x - this->getAbsoluteBounds().y;
	    int yy =y - this->getAbsoluteBounds().y;


		this->setState(SP_STATE_ACTIVE);
		this->notify(string("mousedown"), this, me);
		for(vector<SPNode *>::iterator it = this->getChildNodes().begin(); it != this->getChildNodes().end(); ++it) {
			SPElement *elm = static_cast<SPElement *>(*it);

			if(!elm->isVisible()) continue;

	      	if(x > elm->getAbsoluteBounds().x && x < elm->getAbsoluteBounds().x + elm->getAbsoluteBounds().width &&
				y > elm->getAbsoluteBounds().y && y < elm->getAbsoluteBounds().y + elm->getAbsoluteBounds().height) {
				elm->mouseDown(sender, me);
			}
		}
	}
	void SPElement::mouseUp(SPObject* sender, SPEventArgs* e) {
		SPMouseEventArgs *me = dynamic_cast<SPMouseEventArgs *>(e);
		int x = me->getX();
		int y = me->getY();

		this->notify(string("mouseup"), this, me);
		this->setState(SP_STATE_NORMAL);
		for(vector<SPNode *>::iterator it = this->getChildNodes().begin(); it != this->getChildNodes().end(); ++it) {
			SPElement *elm = static_cast<SPElement *>(*it);

			if(!elm->isVisible()) continue;

	     	if(x > elm->getAbsoluteBounds().x && x < elm->getAbsoluteBounds().x + elm->getAbsoluteBounds().width &&
				y > elm->getAbsoluteBounds().y && y < elm->getAbsoluteBounds().y + elm->getAbsoluteBounds().height) {
				elm->mouseUp(sender, e);
			}
		}
	}
	void SPElement::addClass(string value) {
	    this->m_classList.push_back(value);
	}

	bool SPElement::hasClass(string value) {
	   return  std::find(this->m_classList.begin(), this->m_classList.end(), value) != this->m_classList.end(); // @suppress("Function cannot be instantiated")
	}

	void SPElement::removeClass(string value) {
	    this->m_classList.erase(std::find(this->m_classList.begin(), this->m_classList.end(), value), this->m_classList.end());
	}

	SPFontStyle SPElement::getFontStyle() {
	    return this->fontStyle;
	}
	void SPElement::applyStylesheet(SPStylesheet *style) {

	}

	SPColor SPElement::getBackgroundColor()
	{
		SPColor def = SPColor(0, 0, 0, 0);
		if (auto tint = this->getTintColor(); tint.has_value())
		{
			if (this->getColorScheme() == SP_COLOR_SCHEME_DARK) {
				def = *tint;
			} else {
				float t = float(tint->getA()) / 255.0f * 0.25f;
				def = def.mix(*tint, t);
			}
			def = def.alpha(1);
		}
		switch (this->getColorScheme())
		{
		case SP_COLOR_SCHEME_LIGHT:
			def = def.lighten(0.3);
			break;
		case SP_COLOR_SCHEME_DARK:
			def = def.lighten(-0.1);
			break;
		default:
			break;
		}
		return m_backgroundColor.value_or(def);
	}

	SPColor SPElement::getForegroundColor()
	{
		SPColor def = SPColor(28, 28, 28, 255);
		if (this->getColorScheme() == SP_COLOR_SCHEME_DARK)
		{
			def = SPColor(200, 200, 200, 255);
		}
		if (auto tint = this->getTintColor(); tint.has_value())
			def = def.tint(*tint);
		return m_foregroundColor.value_or(def);

	}

	void SPElement::applyColorAttributeFromSkin (string attr, string prop) {
	    if (this->m_skin->colors.find(prop) != this->m_skin->colors.end()) {
	        pixel *color = this->m_skin->colors[prop];

	        this->set(attr, space_pixel_to_hex(color));
	    }
	}

	void SPElement::applyImageAttributeFromSkin (string attr, string prop) {
	    if (this->m_skin->images.find(prop) != this->m_skin->images.end()) {
	        SPImage *img = this->m_skin->images[prop];

	        this->setObject(attr, (void *)img);
	    }
	}

	void SPElement::generateAndSetDefaultGradient()
	{
		this->setGradient(this->generateDefaultGradient());
	}

	SPGradient SPElement::generateDefaultGradient()
	{
		SPColor bgColor = this->getColorAttribute("bgcolor", SPColor(28, 28, 28, 255));
		SPColor fgColor = this->getColorAttribute("fgcolor", SPColor(255, 255, 255, 255));

		SPColor startColor = bgColor.lighten(0.1);
		SPColor endColor = bgColor.lighten(-0.1);
		return SPGradient(startColor, endColor, 0);
	}

	SPElement::SPElement(SPElement *parent) : SPNode(static_cast<SPNode*>(parent)), fontStyle("Tahoma", 11, 1, false, false) {
		//this->setTintColor(new SPColor(0, 127, 255, 56));
		this->setScrollable(false);
		if (parent != nullptr) {
			this->setParentNode(parent);
			this->fontStyle = parent->fontStyle;
			this->absoluteBounds = { parent->getX(), parent->getY(), parent->getWidth(), parent->getHeight() };
		}
		this->visible = true;
		this->scrollX = 0;
		this->scrollY = 0;
	    this->data = "";
		if (this->m_skin != nullptr) {
			this->applySkin();
		}
		this->x = 0;
		this->y = 0;
		this->z = 0;
		this->m_width = 0;
		this->m_height = 0;
		this->margins = { 0, 0, 0, 0 };
		this->padding = { 0, 0, 0, 0 };
		this->addEventListener("attributechanged", SPCallback(&SPElement::onAttributeChanged), this, this);
	}

	void SPElement::request(const string& method, const string &uri, const string &data) {
		const string prefix = "spacify:";
		if (uri.rfind(prefix, 0) != 0) {
			this->loadXUL("./resources/app/views/error.xml");
			return;
		}

		string route = uri.substr(prefix.size());
		const size_t separator = route.find(':');
		if (separator != string::npos)
			route.resize(separator);
		if (route != "start" && route != "artist") {
			this->loadXUL("./resources/app/views/error.xml");
			return;
		}

		const string filename = "./resources/app/views/" + route + ".xml";
		std::ifstream templateFile(filename, std::ios::binary);
		if (!templateFile) {
			this->loadXUL("./resources/app/views/error.xml");
			return;
		}

		std::ostringstream templateSource;
		templateSource << templateFile.rdbuf();
		nlohmann::json requestData = nlohmann::json::object();
		if (!data.empty()) {
			try {
				nlohmann::json parsed = nlohmann::json::parse(data);
				if (parsed.is_object())
					requestData = std::move(parsed);
			} catch (const std::exception&) {
			}
		}

		string action;
		if (requestData.contains("command") && requestData.at("command").is_string())
			action = requestData.at("command").get<string>();

		std::time_t now = std::time(nullptr);
		std::tm localTime{};
#ifdef _WIN32
		localtime_s(&localTime, &now);
#else
		localtime_r(&now, &localTime);
#endif
		std::ostringstream timestamp;
		timestamp << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
		nlohmann::json model = nlohmann::json::object();
		model["now"] = timestamp.str();

		try {
			this->setInnerXUL(space::SPTemplate::render(
				templateSource.str(), method, uri, action, requestData, model));
		} catch (const std::exception& error) {
			if (s_xulErrorHandler)
				s_xulErrorHandler(error.what());
			this->loadXUL("./resources/app/views/error.xml");
		}
	}

	void SPElement::onLoad() {
	}

	void SPElement::loadXUL(const string &filename) {
		// Safely load a XUL document from disk without recursively retrying the
		// error view when it is also unavailable.
		//
		// To avoid this:
		//  - We only attempt to fall back to the error view when we are NOT
		//    already trying to load it.
		//  - If loading the error view itself fails, we just return quietly.
		//
		std::ifstream file(filename, std::ios::binary);
		std::string contents((std::istreambuf_iterator<char>(file)),
							 std::istreambuf_iterator<char>());
		SPXmlNode root;
		if (file && parseSPXml(contents, root)) {
			this->loadXULFromXMLDoc(root);
		} else {
			// Avoid infinite recursion if the error view can't be loaded either.
			const std::string kErrorViewPath = "./resources/app/views/error.xml";
			if (filename != kErrorViewPath) {
				if (s_xulErrorHandler)
					s_xulErrorHandler("An error occurred");
				this->loadXUL(kErrorViewPath);
			}
			// If we are already trying to load the error view, give up silently.
		}
	}

	void SPElement::setInnerXUL(const string &uri) {
		SPXmlNode root;
		if (parseSPXml(uri, root))
			this->loadXULFromXMLDoc(root);
	}
	SPElement *SPElement::getSlotElement() {
		return this;
	}
	void SPElement::loadXULFromXMLDoc(const SPXmlNode& root) {
		this->clear();
		string thisType = this->getType();

		SPElement *slot = this->getSlotElement();
		for (const auto& attribute : root.attributes)
			slot->set(attribute.first, attribute.second);
		for (const auto& child : root.children) {
			if (child.textNode)
				continue;
			SPElement *elm = this->parseElement(child, slot, this);
			slot->appendChild(elm);
		}
		this->pack();
		this->invalidate();
		string type = this->getType();
		cout << type << endl;
		this->notify("load", this, new SPEventArgs(this));
	}
	SPElement* SPElement::getTabBarViewElement() {
		for (auto & it : this->children) {
			SPElement *elm = (SPElement*) it;
			if (elm->getType() == "tabbarview") {
				return (SPElement *)it;
			}
		}
		return nullptr;
	}
	SPElement* SPElement::getViewStackElement() {
		for (auto & it : this->children) {
			SPElement *elm = (SPElement *)it;
			if (elm->getType() == "viewstack") {
				return (SPElement *)it;
			}
		}
		return nullptr;
	}
	void SPElement::loadSrc(const string &uri) {
		// Create the request object
		if (uri.empty()) {
			return;
		}
		this->setStringAttributeSilently("src", uri);
		this->notify("unload", this, new SPEventArgs(this));
		this->request("GET", uri, "");
	}

	void SPElement::onAttributeChanged(SPObject* sender, SPEventArgs* e) {
		SPAttributeChangedEventArgs *args = dynamic_cast<SPAttributeChangedEventArgs *>(e);
		if (args->getAttributeName() == "bgcolor" || args->getAttributeName() == "fgcolor") {
			this->invalidate();
		}
		if (args->getAttributeName() == "flex") {
			this->m_flex = atoi(args->getNewValue().c_str());
		}
		if (args->getAttributeName() == "gap") {
			this->m_gap = atoi(args->getNewValue().c_str());
		}
		if (args->getAttributeName() == "width") {
			this->setWidth(atoi(args->getNewValue().c_str()));
		}
		if (args->getAttributeName() == "height") {
			this->setHeight(atoi(args->getNewValue().c_str()));
		}
		if (args->getAttributeName() == "src") {
			this->loadSrc(args->getNewValue());
		}
	}

	void SPElement::applySkin() {
		this->applyColorAttributeFromSkin(string("bgcolor"), string("body.background.color"));
		this->applyColorAttributeFromSkin(string("fgcolor"), string("body.foreground.color"));

		string type = string(this->getType());

		this->applyColorAttributeFromSkin(string("bgcolor"), string(this->getType()) + "body.background.color");
		this->applyColorAttributeFromSkin(string("fgcolor"), string(this->getType()) + "body.foreground.color");
		if (this->m_skin->hasImage(this->SPElement::getType() + ".background.image")) {
			this->m_backgroundImage = this->m_skin->getImage(string(this->getType()) + ".background.image", "");
		}
	}

	void SPElement::setBackgroundImage(SPImage* value)
	{
		this->m_backgroundImage = value;
	}

	nlohmann::json SPElement::getFormData() {
		nlohmann::json formData = nlohmann::json::object();
		for (auto & it : this->children)
		{
			auto *elm = dynamic_cast<SPElement *>(it);
			if (elm->getType() == "input") {
				SPInputElement *input = (SPInputElement *)elm;
				formData[input->getStringAttribute("name", "")] = input->getValue();
			}
			auto subFormData = elm->getFormData();
			for (auto item : subFormData.items()) {
				formData[item.key()] = item.value();
			}
		}
		return formData;
	}

	SPElement * SPElement::getDeepestElement(const int level) {
		for (const auto child : this->children) {
			return static_cast<SPElement *>(child)->getDeepestElement(level + 1);
		}
		return this;
	}

	SPWindowElement *SPElement::getWindowElement() {
		SPElement *parent = this->getParentElement();
		while (parent != nullptr) {
			// cout << "Parent type: " << parent->getType() << "\r\n";
			if (parent->getType() == "window") {
				return (SPWindowElement *)parent;
			}
			if (parent == parent->getParentElement()) {
				throw "Parent cant be this";
			}
			parent = parent->getParentElement();

		}
		return nullptr;
	}

	void SPElement::invalidate()
	{
	    auto rect = this->getAbsoluteBounds();
	    auto *we = this->getWindowElement();
	    if (we) we->invalidateRegion(rect);
	}
	sp_margin SPElement::getMargins() {
		return this->margins;
	}
	sp_margin SPElement::getPadding() {
		return this->padding;
	}

	void SPElement::setMargins(sp_margin value) {
		this->margins = value;
	}
	void SPElement::setPadding(sp_margin value) {
		this->padding = value;
	}

	SPElement::~SPElement() {
	}
	int SPElement::getHeight() {
		return this->m_height;
	};
	int SPElement::getWidth() {
		return this->m_width;
	}
	int SPElement::getX() {
		return this->x;
	}
	void SPElement::setColorScheme(short value)
	{
		this->m_colorScheme	= value;
	}

	void SPElement::setId(const string &value) {
		SPNode::setId(value);
	}

	short SPElement::getColorScheme() {
		short colorScheme = this->m_colorScheme;
		if (colorScheme == SP_COLOR_SCHEME_DEFAULT)
		{
			if (this->getParentElement() != nullptr)
			{
				colorScheme = this->getParentElement()->getColorScheme();
				if (colorScheme != SP_COLOR_SCHEME_DEFAULT) {
					return colorScheme;
				}
			}
			else if (this->getWindowElement() != nullptr)
			{
				colorScheme = this->getWindowElement()->getColorScheme();
			}
		}
		if (colorScheme < 1 || colorScheme > 2)
		{
			throw "Invalid color scheme";
		}
		return colorScheme;
	}

	void SPElement::processEvent(const SPEvent& event)
	{
		if (event.type == "mousedown")
		{
			auto *mouseEventArgs = dynamic_cast<SPMouseEventArgs *>(event.args);
			if (mouseEventArgs != nullptr)
			{
				if (mouseEventArgs->getX() > this->getAbsoluteBounds().x && mouseEventArgs->getX() < this->getAbsoluteBounds().x + this->getWidth() &&
					mouseEventArgs->getY() > this->getAbsoluteBounds().y && mouseEventArgs->getY() < this->getAbsoluteBounds().y + this->getHeight())
				{
					this->dispatchEvent("mousedown", mouseEventArgs, this);
				}

			}
		}
		if (event.type == "mouseup")
		{
			auto *mouseEventArgs = dynamic_cast<SPMouseEventArgs *>(event.args);
			if (mouseEventArgs != nullptr)
			{
				if (s_capturedElement == this)
				{
					this->dispatchEvent("mouseup", mouseEventArgs, this);
				}
				else if (mouseEventArgs->getX() > this->getAbsoluteBounds().x && mouseEventArgs->getX() < this->getAbsoluteBounds().x + this->getWidth() &&
					mouseEventArgs->getY() > this->getAbsoluteBounds().y && mouseEventArgs->getY() < this->getAbsoluteBounds().y + this->getHeight())
				{
					this->dispatchEvent("mouseup", mouseEventArgs, this);
				}
			}
		}
		if (event.type == "mousemove")
		{
			auto *mouseEventArgs = dynamic_cast<SPMouseEventArgs *>(event.args);
			if (mouseEventArgs != nullptr)
			{
				if (s_capturedElement == this)
				{
					this->dispatchEvent("mousemove", mouseEventArgs, this);
				}
				else if (mouseEventArgs->getX() > this->getAbsoluteBounds().x && mouseEventArgs->getX() < this->getAbsoluteBounds().x + this->getWidth() &&
					mouseEventArgs->getY() > this->getAbsoluteBounds().y && mouseEventArgs->getY() < this->getAbsoluteBounds().y + this->getHeight())
				{
					this->dispatchEvent("mousemove", mouseEventArgs, this);
				}
			}
		}
		for (auto & it : this->children)
		{
			auto *elm = dynamic_cast<SPElement *>(it);
			if(!elm->isVisible()) continue;
			elm->processEvent(event);
		}
	}

	void SPElement::processMessage(const SPMessage& message)
	{
		for (auto & it : this->children)
		{
			auto *elm = dynamic_cast<SPElement *>(it);

			if (message.type == "event") {
				if (message.predicate == "mousedown" || message.predicate == "mouseup" || message.predicate == "mousemove") {
					//this->dispatchEvent(message.predicate, args, this);
				}
			}

			elm->processMessage(message);
		}
	}

	int SPElement::getY() {
		return this->y;
	}
	int SPElement::getZ() {
		return this->z;
	}
	void SPElement::setWidth(int w) {
		if (w < 1)
		{
			w = 1;
		}
		this->m_width = w;
	}
	void SPElement::setHeight(int h) {
		if (h < 1)
		{
			h = 1;
		}
		this->m_height = h;

	}
	void SPElement::setX(int x) {
		if (x < 0)
		{
			x = 0;
		}
		this->x = x;
	}
	void SPElement::setY(int y) {
		if (y < 0)
		{
			y = 0;
		}
		this->y = y;
	}
	void SPElement::setZ(int z) {
		this->z = z;
	}

	std::string SPElement::getType() {
	    return "element";
	}

	void SPElement::setUri(const string& uri) {
		SPNode::setUri(uri);
		if (this->getViewStackElement() != nullptr) {
			this->getViewStackElement()->setUri(uri);
		}
	}

	void SPElement::paint(int x, int y, int width, int height, SPGraphicsContext *c)
	{
		SPColor fgColor = this->getForegroundColor();
		SPColor bgColor = this->getBackgroundColor();
		int fontSize = this->getIntegerAttribute("size", 11);
		string fontFamily = this->getStringAttribute("font", "Tahoma");

		string stateClass;
		switch (this->getState()) {
		case SP_STATE_ACTIVE:
			stateClass = "active";
			break;
		case SP_STATE_HOVER:
			stateClass = "hover";
			break;
		default:
			stateClass = "";
		}

		if (!this->getSkin()->drawElement(c, this, stateClass, sp_rectangle { x, y, width, height, 0 }))
		{
			if (bgColor.getA() > 0) {
				c->fillRectangle(x, y, width, height, bgColor);
			} else {
				cout << "Transparent background for element: " << this->getType() << "\r\n";
			}
		}
		SPColor color(255, 0, 0, 255);
		string text = this->getInnerText();
		// draw text
		if (!text.empty())
			c->drawString(text, SPFontStyle(fontFamily, fontSize, fontSize / 2, false, false), fgColor, x - scrollX, y - scrollY, this->getWidth(), this->getHeight());
		// adjust for scroll
	}

	void SPElement::resizeToText(bool width, bool height) {
		sp_rectangle bounds = this->getTextBounds();
		if (width) {
			this->setWidth(bounds.width * 4);
		}
		if (height) {
			this->setHeight(bounds.height * 2);
		}
	}

	sp_rectangle SPElement::getTextBounds() {
		SPGraphicsContext *c = this->createGraphics();
		string text = this->getInnerText();
		if (text.empty()) {
			return sp_rectangle { 0, 0, 0, 0 };
		}
	    sp_rectangle tSize = c->measureString(text, fontStyle);
		return tSize;
	}

	string SPElement::getSrc() {
		return this->getStringAttribute("src", "");
	}
	SPColor SPElement::getColorAttribute(const string& attrName, SPColor defaultValue) {
		if (attrName == "bgcolor") {
			return this->getBackgroundColor();
		} else if (attrName == "fgcolor") {
			return this->getForegroundColor();
		} else {
			return this->SPNode::getColorAttribute(attrName, defaultValue);
		}
	}

    void SPElement::setColorAttribute(const string& attrName, SPColor value) {
    	SPColor oldValue = SPColor(0, 0, 0, 0);
		if (attrName == "bgcolor") {
			oldValue = this->getBackgroundColor();
			this->m_backgroundColor = value;
		} else if (attrName == "fgcolor") {
			oldValue = this->getForegroundColor();
			this->m_foregroundColor = value;
		} else {
			SPNode::setColorAttribute(attrName, value);
			return;
		}
    }


	void SPElement::refresh() {
		this->setAttribute("src", this->getSrc());
	}

	void SPElement::setSrc(const string &value) {
		this->setStringAttribute("src", value);
	}

	void SPElement::paint(int x, int y, SPGraphicsContext *c) {
	    if (!this->isVisible()) {
	        return;
	    }
		sp_position pos;
		std::vector<SPNode *> children = this->getChildNodes();
		x += this->getX();
		y += this->getY();
		int width =  this->getWidth();
		int height = this->getHeight();
	    this->absoluteBounds = {
			x, y, width, height
		};



		sp_rectangle rect2;
		rect2.x = this->absoluteBounds.x;
		rect2.y = this->absoluteBounds.y;
		rect2.width = width;
		rect2.height = height;
		if (this->clipView) {
	        c->setClip(absoluteBounds);
		}
		x -= this->scrollX;
		y -= this->scrollY;
		this->paint(rect2.x, rect2.y, rect2.width, rect2.height, c);
		SPColor borderColor = this->getBorderColor();
		if (!borderColor.empty() && this->drawBorder) {
			c->drawRectangle(x, y, width, height, borderColor);
		}
		for(std::vector<SPNode *>::iterator it = children.begin(); it != children.end(); ++it) {
			SPElement *elm = static_cast<SPElement *>(*it);
			if(elm != nullptr) {
				elm->paint(rect2.x, rect2.y, c);
			}
		}
		SPColor bgColor = this->getColorAttribute("bgcolor", SPColor(28, 28, 28, 255));
		SPColor fgColor = this->getColorAttribute("fgcolor", SPColor(255, 255, 255, 255));
		int fontSize = this->getIntegerAttribute("size", 11);
		string fontFamily = this->getStringAttribute("font", "Tahoma");
		/*
		c->drawRectangle(rect2.x, rect2.y, width, height, SPColor(255, 0, 0,255));
		c->drawString(this->getType(), SPFontStyle(fontFamily, fontSize, fontSize / 2, false, false), fgColor, rect2.x, rect2.y, rect2.width,rect2.height);
		const string strLoc = "type=" + this->getType() + "&x=" + std::to_string(x) + "&y=" + std::to_string(y) + "&width=" + std::to_string(width) + "&height=" + std::to_string(height) + "&flex=" + std::to_string(this->getFlex());
		c->drawString(strLoc, SPFontStyle(fontFamily, fontSize, fontSize / 2, false, false), SPColor(0, 0, 255, 255), rect2.x + (rect2.width / 2), rect2.y + (rect2.height / 2), rect2.width,rect2.height);
		*/
		if (clipView) {
	        c->declipRect();
		}
	}

	void SPElement::pack()
	{
		if (!this->getInnerText().empty()) {
			this->resizeToText(false, true);
		}
		if (this->getUnitWidth().value > 0) {
			switch (this->getUnitWidth().unit) {
			case sp_size_scale::undefined:
				break;
			case sp_size_scale::percent:
				this->setWidth((int)(this->getUnitWidth().value / this->getParentElement()->getWidth()));
				break;
			case sp_size_scale::pixel:
				this->setWidth(this->getUnitWidth().value);
				break;
			}
		}
		if (this->getUnitHeight().value > 0) {
			switch (this->getUnitHeight().unit) {
			case sp_size_scale::undefined:
				break;
			case sp_size_scale::percent:
				this->setHeight((int)(this->getUnitHeight().value / this->getParentElement()->getHeight()));
				break;
			case sp_size_scale::pixel:
				this->setWidth(this->getUnitHeight().value);
				break;
			}
		}
		for (auto child : this->getChildNodes()) {
			SPElement *elm = dynamic_cast<SPElement *>(child);
			elm->pack();
		}
		//cout << "Element: " << this->getType() << "#" << this->getId() << " packed to width: " << this->getWidth() << " height: " << this->getHeight() << " flex: " << this->getFlex() << endl;
	}

	void SPElement::resize(int width, int height)
	{
		this->m_width = width;
		this->m_height = height;
		this->invalidate();
		this->pack();

		//cout << "Element: " << this->getType() << "#" << this->getId() << " resized to width: " << width << " height: " << height << " flex: " << this->getFlex() << endl;
	}

	void SPElement::scroll(int scrollX, int scrollY, int x, int y) {
	    int xx = x - this->getAbsoluteBounds().y;
	    int yy = y - this->getAbsoluteBounds().y;
	    // cout << "Current scrolling element is " << this->getType() << endl;
	    bool foundElement = false;
	    if (string(this->getType()) == string("toplistview")) {
				//cout << "Test" << endl;
	    }
			for(vector<SPNode *>::iterator it = this->getChildNodes().begin(); it != this->getChildNodes().end(); ++it) {
				if ((this->getWindowElement())->getDeepestElement() == nullptr) {
						break;
				}
				SPElement *elm = dynamic_cast<SPElement *>(*it);
				string type = elm->getType();
				if (elm->isVisible()) {
					if(x > elm->getAbsoluteBounds().x && x < elm->getAbsoluteBounds().x + elm->getAbsoluteBounds().width &&
						y > elm->getAbsoluteBounds().y && y < elm->getAbsoluteBounds().y + elm->getAbsoluteBounds().height) {
						foundElement = true;
						elm->scroll(scrollX, scrollY, x, y);
					}
				}
		}
	}

	void SPElement::scroll(int x, int y) {
	    if (this->clipView) {
	        this->scrollX += x;
	        this->scrollY += y;
	        if (scrollY < 0) {
	            scrollY = 0;
	        }
	        this->invalidate();
	    }
	}

	void SPElement::scrollTo(int x, int y) {
	    if (this->clipView) {
	        this->scrollX = x;
	        this->scrollY = y;

	        if (scrollY < 0) {
	            scrollY = 0;
	        }
	        this->invalidate();

	    }

	}

	SPElement *SPElement::parseElement(const SPXmlNode& xmlNode, SPElement *parent,
							   SPElement *document) {
		SPElement *elm = nullptr;
		/*
		string elementId = xmlNode.attributes.at("id");
		if (!elementId.empty()) {
			elm = document->getElementById(elementId);
			assert(elm != nullptr && elm->getParentElement() != parent);
		}*/
		if (elm == nullptr) {
			elm = createElement(xmlNode.name, parent);
		}
		assert(elm->getParentElement() != nullptr);
		elm->setVisible(true);
		for (const auto& attribute : xmlNode.attributes) {
			elm->set(attribute.first, attribute.second);
			if (attribute.first == "hidden") {
				elm->setVisible(false);
			}
			if (attribute.first == "onclick") {
				if (elm->getType() == "button") {
					elm->addEventListener("click", SPCallback(&SPElement::onPostBackButtonClick), this, elm);
				}
			}
		}
		for (const auto& child : xmlNode.children) {
			if (child.textNode) {
				elm->getSlotElement()->setInnerText(child.text);
			} else {
				SPElement *childElement = parseElement(child, elm->getSlotElement(), document);
				elm->getSlotElement()->appendChild(childElement);
				this->pack();
				// cout << "Packed";
			}
		}

		elm->pack();
		return elm;
	}

	void SPElement::onPostBackButtonClick(SPObject *sender, SPEventArgs *event) {
		SPMouseEventArgs *args = dynamic_cast<SPMouseEventArgs*>(event);
		SPButtonElement *button = dynamic_cast<SPButtonElement*>(sender);
		nlohmann::json formData = this->getFormData();
		formData["command"] = button->getStringAttribute("onclick", "submit");
		this->postBack(formData);
	}

	SPElement * SPElement::createElement(const string& tagName, SPElement *parent) {
		assert(parent != nullptr);
		SPElement *elm = nullptr;
		if (tagName == "text") {
			elm = new SPTextElement(parent);
		} else if (tagName == "img") {
			elm = new SPImageElement(parent);
		} else if (tagName == "button") {
			elm = new SPButtonElement(parent);
		} else if (tagName == "input") {
			elm = new SPInputElement(parent);
		} else if (tagName == "view") {
			elm = new SPTabBarViewElement(parent);
		} else if (tagName == "vbox") {
			elm = new SPVBoxElement(parent);
		} else if (tagName == "splitter") {
			elm = new SPSplitterElement(parent);
		} else if (tagName == "hbox") {
			elm = new SPHBoxElement(parent);
		} else if (tagName == "section") {
			elm = new SPSectionElement(parent);
		} else {
			elm = new SPElement(parent);
		}
		assert(elm->getParentElement() != nullptr);
		elm->setTagName(tagName);
		return elm;
	}

	SPElement* SPElement::getElementById(const string& id) const {
		for (const auto child : this->children) {
			if (child->getStringAttribute("id", "") == id) {
				return (SPElement *)child;
			}
			SPElement *grandChildNode = ((SPElement *)child)->getElementById(id);
			if (grandChildNode != nullptr) {
				return grandChildNode;
			}
		}
		return nullptr;
	}

}
