#ifndef EVENTARGS_H
#define EVENTARGS_H
#include "SPNode.h"
#include "SPObject.h"
namespace space {
class SPObject;
class SPNode;
class SPEventArgs : public SPObject
{
protected:
	SPObject *sender;
public:
	explicit SPEventArgs(SPObject *sender) {
		this->sender = sender;
	}
    SPEventArgs();
    ~SPEventArgs() override;
	SPObject *getSender() const {
		return sender;
	}
	void setSender(SPObject *value) {
		this->sender = value;
	}
};
class SPNodeTreeEventArgs : public SPEventArgs {
	SPNode *node;
	SPNode *parent;
	string action;
public:
	SPNodeTreeEventArgs(SPNode *node, SPNode *parent, const string& action);
	SPNode *getNode() const {
		return node;
	}
	SPNode *getParent() const {
		return parent;
	}
	void setAction(const string& value) {
		this->action = value;
	}
	string getAction() {
		return action;
	}
};
class SPAttributeChangedEventArgs : public SPEventArgs {
private:
	string attributeName;
	string newValue;
	string oldValue;
	string attributeType;
public:
	SPAttributeChangedEventArgs(const string &attributeName,
	                            const string &newValue, const string &oldValue, const string &attributeType);

	void setAttributeName(const string& value) {
		this->attributeName = value;
	}
	void setNewValue(const string& value) {
		this->newValue = value;
	}
	void setOldValue(const string& value) {
		this->oldValue = value;
	}
	string getAttributeType() {
		return attributeType;
	}
	void setAttributeType(const string& value) {
		this->attributeType = value;
	}
	string getAttributeName() {
		return attributeName;
	}
	string getNewValue() {
		return newValue;
	}
	string getOldValue() {
		return oldValue;
	}
};

class SPKeyEventArgs : public SPEventArgs {
    int  m_keyCode;
    bool m_cmdDown;
public:
    SPKeyEventArgs(int keyCode, bool cmdDown)
        : SPEventArgs(), m_keyCode(keyCode), m_cmdDown(cmdDown) {}
    int  getKeyCode()  const { return m_keyCode; }
    bool getCmdDown()  const { return m_cmdDown; }
};
}

#endif // EVENTARGS_H
