#include "SPTabBarElement.h"

#include <iostream>

#include "SPMouseEventArgs.h"
#include "SPColor.h"
#include "SPSkin.h"
#include "SPGraphicsContext.h"

namespace space {

SPTab::SPTab(SPTabBarElement *tabBar, string name, string id, string uri) {
    this->tabBar = tabBar;
    this->name = name;
    this->id = id;
    this->uri = uri;
}

int SPTabBarElement::tabbar_mousedown(SPObject *sender, SPEventArgs *e) {
    SPTabBarElement *tabBar = (SPTabBarElement *)sender;
    SPMouseEventArgs *me = (SPMouseEventArgs *)e;
    int x = me->getX() - tabBar->getAbsoluteBounds().y;
    int y = me->getY() - tabBar->getAbsoluteBounds().y;
    int left = 0;
    ;
    for(std::vector<SPTab *>::iterator it = tabBar->tabs.begin(); it != tabBar->tabs.end(); ++it) {
        SPTab *tab = (SPTab *)dynamic_cast<SPTab *>(*it);
        string title = tab->getName();
        string id = tab->getId();
        string uri = tab->getUri();
        SPFontStyle fontStyle = tabBar->getFontStyle();
        SPGraphicsContext *g = tabBar->createGraphics();
        sp_rectangle strSize = g->measureString(title.c_str(), fontStyle);
        int width = 20 + strSize.width + 20;
        if (x > left && x < left + width) {
            tabBar->activeTab = tab;
            TabBarEventArgs *args = new TabBarEventArgs(tab);
            tabBar->notify(string("tabselected"), (SPObject *)tabBar, (SPEventArgs *)args);
        }

        left += width;
    }
    tabBar->invalidate();
    return 0;
}
string SPTabBarElement::getType() {
     return "tabbar";
}
void SPTabBarElement::addTab(const string& id, const string& name, const string& uri) {
    SPTab *tab = new SPTab(this, id, name, uri);
    this->tabs.push_back(tab);
    if (this->tabs.size() == 1) {
        this->activeTab = tab;
    }
}

void SPTabBarElement::remove(SPTab *tab) {
    this->tabs.erase(std::remove(this->tabs.begin(), this->tabs.end(), tab), this->tabs.end());
}

void SPTabBarElement::remove(const string& uri) {
	for (std::vector<SPTab *>::iterator it = this->tabs.begin(); it != this->tabs.end(); ++it) {
		SPTab *tab = (SPTab *)static_cast<SPTab *>(*it);
		if (tab->getUri() == uri) {
			this->remove(tab);
			break;
		}
	}
}

TabBarEventArgs::TabBarEventArgs(SPTab *tab) {
    this->tab = tab;
}

void SPTabBarElement::clear() {
	this->tabs.clear();
}

SPTabBarElement::SPTabBarElement()
 : SPElement()
{
    this->clipView = false;
    //ctor
    this->addEventListener(string("mousedown"), SPCallback(&SPTabBarElement::tabbar_mousedown), this, this);
}
SPTabBarElement::SPTabBarElement(SPElement *parent)
 : SPElement(parent)
{
	this->setTagName("tabbar");
    //ctor
    this->addEventListener(string("mousedown"), SPCallback(&SPTabBarElement::mouseDown), this, this);
    std::vector<SPObserver *> observers = this->observers;
}

void SPTabBarElement::addToolButton(const string& id, const string& label, SPImage* icon) {
    m_toolButtons.push_back({id, label, icon});
}

void SPTabBarElement::removeToolButton(const string& id) {
    m_toolButtons.erase(
        std::remove_if(m_toolButtons.begin(), m_toolButtons.end(),
                       [&id](const SPTabToolButton& b) { return b.id == id; }),
        m_toolButtons.end());
}
void SPTabBarElement::mouseDown(SPObject* sender, SPEventArgs *e) {
    SPMouseEventArgs *me = (SPMouseEventArgs *)e;
    SPTabBarElement *tabBar = this;
    int x = me->getX();
    SPFontStyle font = tabBar->getFontStyle();
    SPGraphicsContext *g = tabBar->createGraphics();

    // ── Tab hit-test (left → right) ───────────────────────────────────────────
    bool tabHit = false;
    int left = tabBar->getAbsoluteBounds().x;
    for (auto* tab : tabBar->getTabs()) {
        sp_rectangle strSize = g->measureString(tab->getName(), font);
        int width = 20 + strSize.width + 20;
        if (x > left && x < left + width) {
            tabBar->activeTab = tab;
            TabBarEventArgs *args = new TabBarEventArgs(tab);
            tabBar->notify(string("tabselected"), (SPObject *)tabBar, (SPEventArgs *)args);
            tabHit = true;
            break;
        }
        left += width;
    }

    // ── Tool button hit-test (right → left) ───────────────────────────────────
    if (!tabHit && !m_toolButtons.empty()) {
        int toolRight = tabBar->getAbsoluteBounds().x + tabBar->getWidth();
        for (auto it = m_toolButtons.rbegin(); it != m_toolButtons.rend(); ++it) {
            sp_rectangle sz = g->measureString(it->label, font);
            int btnW = it->label.empty() ? tabBar->getHeight() : 8 + sz.width + 8;
            int btnLeft = toolRight - btnW;
            if (x >= btnLeft && x < toolRight) {
                auto* args = new TabBarToolButtonEventArgs(it->id);
                tabBar->notify("toolbuttonclicked", (SPObject *)tabBar, (SPEventArgs *)args);
                delete args;
                break;
            }
            toolRight = btnLeft;
        }
    }

    tabBar->invalidate();
}
#define bungalow2009 true
void SPTabBarElement::paint(int x, int y, int width, int height, SPGraphicsContext *g) {

    this->getSkin()->drawTabBar(g, this, x, y, width, height);

    // ── Tabs (left → right) ───────────────────────────────────────────────────
    int left = 0;
    int index = 0;
    for (std::vector<SPTab *>::iterator it = this->getTabs().begin(); it != this->getTabs().end(); ++it) {
        SPColor fgColor = this->getForegroundColor();
        SPTab *tab = (SPTab *)static_cast<SPTab *>(*it);
        SPColor shadowColor = fgColor.lighten(-10);
    	if (activeTab != tab) {
    		fgColor = fgColor.lighten(-2);
            shadowColor = fgColor.lighten(10);
    	}
        string title = tab->getName();
        string id = tab->getId();
        sp_rectangle strSize = g->measureString(title, this->getFontStyle());
        int tabWidth = 20 + strSize.width + 20;
        this->getSkin()->drawTabBarTab(g, this, tab, index, this->activeTab == tab, x + left, y, tabWidth, height);
        g->drawString(title, this->getFontStyle(), shadowColor, left + x + (tabWidth / 2) - (strSize.width / 2), y + (height / 2) - (strSize.height / 2) - 1, strSize.width, strSize.height);
        g->drawString(title, this->getFontStyle(), fgColor, left + x + (tabWidth / 2) - (strSize.width / 2), y + (height / 2) - (strSize.height / 2) - 2, strSize.width, strSize.height);
        left += tabWidth;
        index++;
    }

    // ── Tool buttons (right → left) ───────────────────────────────────────────
    if (!m_toolButtons.empty()) {
        int toolRight = x + width;
        for (auto it = m_toolButtons.rbegin(); it != m_toolButtons.rend(); ++it) {
            sp_rectangle sz = g->measureString(it->label, this->getFontStyle());
            int btnW = it->label.empty() ? height : 8 + sz.width + 8;
            int btnX  = toolRight - btnW;
            this->getSkin()->drawTabBarToolButton(g, this, &(*it), btnX, y, btnW, height);
            if (!it->label.empty()) {
                SPColor fg = this->getForegroundColor();
                SPColor shadow = fg.lighten(-10);
                int txtX = btnX + (btnW / 2) - (sz.width / 2);
                int txtY = y + (height / 2) - (sz.height / 2);
                g->drawString(it->label, this->getFontStyle(), shadow, txtX, txtY - 1, sz.width, sz.height);
                g->drawString(it->label, this->getFontStyle(), fg,     txtX, txtY - 2, sz.width, sz.height);
            }
            toolRight = btnX;
        }
    }
}


SPTabBarElement::~SPTabBarElement()
{
}

space::SPTab::~SPTab() {   
}
}