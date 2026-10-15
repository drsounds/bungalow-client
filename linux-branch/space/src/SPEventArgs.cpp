#include "SPEventArgs.h"

namespace space {
SPEventArgs::SPEventArgs()
{
    //ctor
}

SPEventArgs::~SPEventArgs()
{
    //dtor
}

SPNodeTreeEventArgs::SPNodeTreeEventArgs(SPNode *node, SPNode *parent,
	const string &action) {
	this->node = node;
	this->parent = parent;
	this->action = action;
}

SPAttributeChangedEventArgs::SPAttributeChangedEventArgs(const string& attributeName,
                                                         const string& newValue, const string& oldValue, const string& attributeType) {
	this->attributeName = attributeName;
	this->newValue = newValue;
	this->oldValue = oldValue;
	this->attributeType = attributeType;
}
}
