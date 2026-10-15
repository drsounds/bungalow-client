#ifndef NODE_H
#define NODE_H
#include "SPObject.h"
#include "SPColor.h"
#include "SPEventArgs.h"
#include "SPObservable.h"

#include <map>
#include <string>
#include <vector>
#include "SPXmlParser.h"
using namespace std;
namespace space {
    class Space;
	class SPWindowElement;
    class SPColor;
    class SPNode : public SPObject, public SPObservable {
    protected:
    	string uri;
        map<string, void *> properties;
        map<string, string> stringAttributes;
        map<string, SPColor> colorAttributes;
        map<string, int> integerAttributes;
        SPNode *m_parent = nullptr;
        string tagName = "node";
    	string m_id;
    	string m_innerText;
    	string m_name;

    public:
    	string getUri();
    	virtual void setUri(const string& value);
    	string getId() const {
    		return this->m_id;
    	}
    	virtual void setId(const string& value);
        string getTagName() const {
	        return this->tagName;
        }
    	void setTagName(const string& value) {
        	this->tagName = value;
        }
    	string getInnerText() const {
	        return this->m_innerText;
        }
        void setInnerText(const string& value);
        SPNode *parseNode(const SPXmlNode& xmlNode, SPNode *parent,
                          SPNode *document);

    	space::Space *getSpace() {
    		if (this->m_space == nullptr) {
    			if (this->getParentNode() != nullptr) {
    				return this->getParentNode()->getSpace();
    			}
    		}
    		return this->m_space;
    	}
    	SPNode *getDeepestNode(const int level);

        std::vector<SPNode *> children;
        SPNode();
        SPNode(Space *space, SPNode *parent) : SPNode() {
	        this->m_parent = parent;
        }
        SPNode(const string &tagName);
        explicit SPNode(SPNode *parent);
        void removeChild(SPNode *child);
        virtual void appendChild(SPNode *child);
        void setInnerXml(const string &value);
        SPNode *createNode(const string &tagName, SPNode *parent);

        void setChildren(const std::vector<SPNode *>& value) {
            this->children = value;
        }


    	void setName(const string& value) {
        	this->setStringAttribute("name",value);
        	this->m_name = value;
        }
    	string getName() {
        	return this->getStringAttribute("name", "");

		}
        SPNode *getNodeById(const string& id) const;
        void setAttribute(const std::string &attrName,
                          const std::string &value);
        virtual void set(const string& attrib, const string& val);
        string get(const string& attrib);
        void *getObject(const string& attrib);
        void setObject(const string& attrib, void *value);
        string getValue();
        SPNode *getParentNode();

        map<string, string>& getStringAttributes() {
            return this->stringAttributes;
        }
        map<string, SPColor>& getColorAttributes() {
            return this->colorAttributes;
        }
        map<string, int>& getIntegerAttributes() {
            return this->integerAttributes;
        }
        map<string, void *>& getProperties() {
            return this->properties;
        }

        void setParentNode(SPNode *parent);
        std::vector<SPNode *>& getChildNodes();
        bool hasAttribute(string attrib);
        virtual SPColor getColorAttribute(const string& title, SPColor defaultValue);
        virtual void setColorAttribute(const string& attrName, SPColor value);
	    int notify(const string &event, SPObject *sender, SPEventArgs *data) override;
	    virtual void clear();
	    int getIntegerAttribute(const string& title, int defaultValue);
        void setIntegerAttribute(const string& title, int value);
        void setStringAttribute(const string& attrName, const string& value);
        string getStringAttribute(const string& attribute, string defaultValue);
	    void setStringAttributeSilently(const string& attrName, const string& value);
    };
}
#endif // NODE_H
