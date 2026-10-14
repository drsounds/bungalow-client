#include "SPNode.h"

#include <map>

#include "SPColor.h"
#include "SPMouseEventArgs.h"
#include "SpaceFrame.h"

#include "SPEventArgs.h"
#include <regex>

using namespace std;

namespace space {
	SPNode::SPNode() {
	}
    SPNode::SPNode(const string& tagName) : SPNode() {
    	this->tagName = tagName;
    }
    SPNode::SPNode(SPNode *parent) : SPNode("node") {
        if (parent == nullptr) {
            return;
        }
    }
	SPNode *SPNode::getDeepestNode(const int level) {
		for (const auto child : this->children) {
			return child->getDeepestNode(level + 1);
		}
		return this;
	}

    SPNode* SPNode::getParentNode() {
        return this->m_parent;
    }
    void SPNode::removeChild(SPNode *child) {
		if (child != nullptr) {
			this->children.erase(std::remove(this->children.begin(), this->children.end(), child), this->children.end());
		}
	}
    string SPNode::getStringAttribute(const string& attrName, string defaultValue) {
		if (!this->stringAttributes.count(attrName)) {
			return defaultValue;
		}
        return this->stringAttributes[attrName];
	}
	void SPNode::setStringAttributeSilently(const string& attrName, const string& value) {
		string oldValue = this->stringAttributes[attrName];
		this->stringAttributes[attrName] = value;

	}
    void SPNode::setStringAttribute(const string& attrName, const string& value) {
    	string oldValue = this->stringAttributes[attrName];
    	this->stringAttributes[attrName] = value;
    	auto *args = new SPAttributeChangedEventArgs(attrName, value, oldValue, "string");
    	this->notify("attributechanged", this, args);

    }
    void SPNode::setColorAttribute(const string& attrName, SPColor value) {
    	SPColor oldValue = SPColor(0, 0, 0, 0);
    	this->colorAttributes[attrName] = value;
    	auto args = new SPAttributeChangedEventArgs(attrName, value.toHTMLColor(), oldValue.toHTMLColor(), "color");
    	this->notify("attributechanged", this, args);
    }

    int SPNode::notify(const string &event, SPObject *sender, SPEventArgs *data)
    {
    	return SPObservable::notify(event, sender, data);

    }

	void SPNode::clear() {
		while (this->children.size() > 0) {
			this->removeChild(this->children.back());
		}
	}

    SPColor SPNode::getColorAttribute(const string& title, SPColor defaultValue) {
    	SPColor color = SPColor(0, 0, 0, 0);

        if (this->colorAttributes.count(title)) {
	        color = this->colorAttributes[title];
        }
		if (color.empty()) {
    		if (this->getParentNode() != nullptr) {
		        color = this->getParentNode()->getColorAttribute(title, defaultValue);
	        }
        }
    	if (color.empty()) {
    		return defaultValue;
    	}
        return color;
    }
    int SPNode::getIntegerAttribute(const string& title, int defaultValue) {
        if (!this->integerAttributes.count(title))
        {
            return defaultValue;
        }
        return this->integerAttributes[title];
    }
    void SPNode::setIntegerAttribute(const string& attrName, int value) {
    	int oldValue = 0;
    	this->integerAttributes[attrName] = value;
    	auto args = new SPAttributeChangedEventArgs(attrName, std::to_string(value), std::to_string(oldValue), "integer");
    	this->notify("attributechanged", this, args);
    }
    std::vector<SPNode *>& SPNode::getChildNodes() {
        return children;
    }
    bool SPNode::hasAttribute(string attr) {
        map<string, void *>::iterator it = this->properties.find(attr);
        return it != this->properties.end();
    }

    void SPNode::setParentNode(SPNode *elm) {
		if (elm == this) {
			throw "Cannot set parent to self";
		}
        this->m_parent = elm;
    }
    std::string SPNode::get(const std::string& title) {
        std::string val = this->stringAttributes[title];
        return val;
    }
    void* SPNode::getObject(const std::string& title) {
        return this->properties[title];
    }

    void SPNode::setObject(const string& attrib, void* value)
    {
        this->properties[attrib] = value;
    }

    string SPNode::getValue()
    {
        return "";
    }


    SPNode* SPNode::getNodeById(const string& id) const {
        for (const auto child : this->children) {
	        if (child->getStringAttribute("id", "") == id) {
		        return child;
	        }
        	for (const auto grandChild : child->getChildNodes()) {
        		SPNode* result = grandChild->getNodeById(id);
        		if (result != nullptr) {
        			return result;
        		}
        	}
        }
    	return nullptr;
    }

    string SPNode::getUri() {
		string uri = this->getStringAttribute("uri", "");
		return uri;
    }

    void SPNode::setUri(const string &value) {
		this->setStringAttribute("uri", value);
    }

	void SPNode::setId(const string& id) {
		this->set("id", id);
	}

	void SPNode::set(const string& title, const string& value) {
		setAttribute(title, value);
	}

    void SPNode::setAttribute(const std::string& attrName, const std::string& value) {
        int n = 0;
		if (attrName == "id") {
			this->m_id = value;
		}
		string oldVal = this->getStringAttribute(attrName, "");
		if (oldVal == value) {
			return;
		}
        this->setStringAttribute(attrName, value);
        try {
        	if (std::regex_match(value, std::regex("^[0-9]+$"))) {
        		n = atoi(value.c_str());
        		setIntegerAttribute(attrName, n);
        	}
        } catch (exception& e) {
            
        }

        if(regex_match(value, std::regex("^#(([0-9a-fA-F]{2}){3}|([0-9a-fA-F]){3})$"))) {
            SPColor c = SPColor(value);
            this->setColorAttribute(attrName, c);
        }
    }

    void SPNode::appendChild(space::SPNode *child) {

        children.push_back(child);
        child->setParentNode(this);
		SPEventArgs *args = new SPNodeTreeEventArgs(child, this, "append");
		this->notify("treechanged", this, args);
    }

	void SPNode::setInnerText(const string& value) {
		this->m_innerText = value;
	}

	SPNode *SPNode::parseNode(const SPXmlNode& xmlNode, SPNode *parent,
								   SPNode *document) {
		if (xmlNode.textNode)
			return parent;
		SPNode *elm = nullptr;
		const auto id = xmlNode.attributes.find("id");
		elm = document->getNodeById(id == xmlNode.attributes.end() ? "" : id->second);
		if (elm == nullptr) {
			elm = createNode(xmlNode.name, parent);
		}
		for (const auto& attribute : xmlNode.attributes)
			elm->set(attribute.first, attribute.second);
		elm->setInnerText(xmlNode.text);
		for (const auto& child : xmlNode.children)
			parseNode(child, elm, document);
		return elm;
	}
	void SPNode::setInnerXml(const string& value) {
		SPXmlNode root;
		if (!parseSPXml(value, root))
			return;
    	SPNode *parsingNode = this;
		for (const auto& child : root.children) {
			if (!child.textNode)
				parsingNode = parseNode(child, parsingNode, this);
    	}
    }

	SPNode * SPNode::createNode(const string& tagName, SPNode *parent) {
    	if (parent == nullptr) {
    		parent = this;
    	}
    	auto *elm = new SPNode(parent);
    	return elm;
    }

}
