#pragma once

#include <string>
#include <vector>

#include "SPEventArgs.h"
#include "SPElement.h"

using namespace std;
namespace space {
class SPElement;
class SPGraphicsContext;
class SPImage;
class SPTabBarElement;

// ── Tool button ───────────────────────────────────────────────────────────────
struct SPTabToolButton {
    string   id;
    string   label;
    SPImage* icon = nullptr;  // optional; caller owns the pointer
};

class TabBarToolButtonEventArgs : public SPEventArgs {
public:
    string buttonId;
    explicit TabBarToolButtonEventArgs(const string& id) : buttonId(id) {}
};

class SPTab {
protected:
public:
    string id;
    string name;
    string uri;
    SPTabBarElement *tabBar;
    SPTabBarElement *getTabBar() {
        return this->tabBar;
    }
    string getId() {
        return this->id;
    }
    string getName() {
        return this->name;
    }
    string getUri() {
        return this->uri;
    }
    SPTab(SPTabBarElement *tabBar, string name, string id, string uri);
    ~SPTab();
};
class SPEventArgs;

class TabBarEventArgs : public SPEventArgs {
public:
    explicit TabBarEventArgs(SPTab *tab);
    SPTab *tab;
};
class SPTabBarElement : public SPElement
{
    public:
		void clear() override;
        SPImage *tabBackgroundImage;
        SPImage *activeTabBackgroundImage;
        int tabbar_mousedown(SPObject* sender, SPEventArgs* e);
        virtual string getType() override;
        SPTabBarElement();
        SPTabBarElement(SPElement *parent);
        void addTab(const string& id, const string& name, const string& uri);
		void remove(SPTab *tab);
		void remove(const string &uri);
		void paint(int x, int y, int width, int height, SPGraphicsContext* c) override;
        virtual ~SPTabBarElement();
        void mouseDown(SPObject* sender, SPEventArgs* e) override;
        vector<SPTab *>& getTabs() {
            return this->tabs;
        }
        // ── Tool buttons (right-aligned in the bar) ───────────────────────────
        void addToolButton(const string& id, const string& label, SPImage* icon = nullptr);
        void removeToolButton(const string& id);
        vector<SPTabToolButton>& getToolButtons() { return m_toolButtons; }

        SPTab *activeTab;
        vector<SPTab *> tabs;
    protected:
    private:
        vector<SPTabToolButton> m_toolButtons;
};
}


