#include "SPTabBarViewElement.h"

#include "SPElement.h"
#include "SPPageElement.h"
#include "SPSectionElement.h"
#include "SPTabBarElement.h"
#include "SPTabStackElement.h"
#include "SPUri.h"
#include "SPViewElement.h"
#include "SPViewStackElement.h"
#include "SPWindowElement.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <iostream>

namespace space {
void SPTabBarViewElement::onLoad(SPObject *sender, SPEventArgs *e) {
	SPViewElement::onLoad();
	if (this->tabBar->getTabs().size() == 0) {
		return;
	}
	SPTab *tab = this->tabBar->getTabs().at(0);
	string strUri = tab->getUri();
	SPUri uri(strUri);
	this->tabStack->setTabId(uri.getHash());
}

void SPTabBarViewElement::setHash(const string &hash) {
	SPViewElement::setHash(hash);
	this->tabStack->setTabId(hash);
}

int SPTabBarViewElement::onTabSelected(SPObject *sender, SPEventArgs *e) {
	TabBarEventArgs *args = (TabBarEventArgs *)e;
	SPTabBarViewElement *view = (SPTabBarViewElement *)sender;
	string strUri = args->tab->getUri();
	SPUri uri(strUri);
	if (view->getWindowElement() != nullptr) {
		view->getWindowElement()->navigate(strUri);
	}
	return 0;
}
SPTabBarViewElement::SPTabBarViewElement() :
	SPViewElement() {}
void SPTabBarViewElement::mouseDown(SPObject *sender, SPEventArgs *e) {
	SPElement::mouseDown(sender, e);
}
SPTabBarViewElement::SPTabBarViewElement(SPElement *parent) :
	SPViewElement(parent) {
	this->setTagName("tabbarview");
	// ctor
	this->setFlex(1);
	this->tabBar = new SPTabBarElement(this);
	this->tabStack = new SPTabStackElement(this);
	this->tabBar->setHeight(28);
	this->appendChild(this->tabBar);
	this->appendChild(this->tabStack);
	this->tabStack->setFlex(1);
	this->tabBar->addEventListener(
		string("tabselected"),
		static_cast<SPCallback>(&SPTabBarViewElement::onTabSelected), this,
		tabBar);
	this->tabStack->addEventListener(
		"treechanged", SPCallback(&SPTabBarViewElement::onTreeChanged), this,
		tabStack);
	this->addEventListener("unload", SPCallback(&SPTabBarViewElement::onUnload),
						   this, this);
	this->addEventListener(
		"mousedown", SPCallback(&SPTabBarViewElement::onClick), this, this);
	this->addEventListener("load", SPCallback(&SPTabBarViewElement::onLoad),
						   this, this);
}
SPElement *SPTabBarViewElement::getViewStackElement() {
	return (SPViewStackElement *)this->tabStack;
}
void SPTabBarViewElement::onClick(SPObject *sender, SPEventArgs *e) {}
void SPTabBarViewElement::clear() {
	this->tabBar->clear();
	this->tabStack->clear();
}
SPElement *SPTabBarViewElement::getSlotElement() { return this->tabStack; }
void SPTabBarViewElement::onUnload(SPObject *sender, SPEventArgs *e) {
	this->tabBar->clear();
}
bool SPTabBarViewElement::acceptsUri(const string &uri) {
	string this_uri = this->getUri();
	bool ret = boost::algorithm::starts_with(uri, this_uri);
	return ret;
}
void SPTabBarViewElement::onTreeChanged(SPObject *sender, SPEventArgs *e) {
	SPTabStackElement *tabStack = (SPTabStackElement *)sender;
	string uri = this->getUri();
	string tagName = tabStack->getTagName();
	cout << "tagName: " << tagName << endl;
	if (uri.empty())
		return;
	SPNodeTreeEventArgs *args = (SPNodeTreeEventArgs *)e;
	if (args->getAction() == "remove") {
		SPElement *elm = dynamic_cast<SPElement *>(args->getNode());
		if (elm->getTagName() == "section") {
			SPViewElement *view = dynamic_cast<SPViewElement *>(elm);
			this->tabBar->remove(view->getUri());
		}
		if (elm->getTagName() == "page") {
			SPViewElement *view = dynamic_cast<SPViewElement *>(elm);
			this->tabBar->remove(view->getUri());
		}
	} else if (args->getAction() == "append") {
		SPElement *elm = dynamic_cast<SPElement *>(args->getNode());
		if (elm->getTagName() == "section") {
			auto *view = dynamic_cast<SPSectionElement *>(elm);
			if (view == nullptr)
				return;
			string viewUri = uri + "#" + view->getId();
			view->setUri(viewUri);
			view->setFlex(1);
			assert(view->getParentElement() == tabStack);
			this->tabBar->addTab(
				elm->getId(), view->getStringAttribute("title", elm->getId()),
				view->getUri());
		}
		if (elm->getTagName() == "page") {
			auto *view = dynamic_cast<SPPageElement *>(elm);
			if (view == nullptr)
				return;
			string viewUri = uri + "#" + view->getId();
			view->setUri(viewUri);
			view->setFlex(1);
			assert(view->getParentElement() == tabStack);
			this->tabBar->addTab(
				elm->getId(), view->getStringAttribute("title", elm->getId()),
				view->getUri());
		}
	}
}
void SPTabBarViewElement::navigate(const string &value) {
	SPUri uri(value);
	this->tabStack->setTabId(uri.getHash());
}
SPTabBarViewElement::~SPTabBarViewElement() {
	delete this->tabBar;
	delete this->tabStack;
}
} // namespace space
