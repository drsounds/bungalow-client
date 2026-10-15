#pragma once
#include "SPUri.h"
#include "SPElement.h"
#include "SPEventArgs.h"
#include <string>
#include <vector>
#include <utility>
#include <map>

using namespace std;

namespace space {
    class SPElement;
    class SPTreeItem;
    class TreeViewEventArgs : public SPEventArgs {
    protected:
        SPTreeItem *item;
        int m_x = 0;
        int m_y = 0;
    public:
        SPTreeItem *getItem() { return this->item; }
        void setItem(SPTreeItem *item) { this->item = item; }
        int getX() const { return m_x; }
        int getY() const { return m_y; }
        void setX(int x) { m_x = x; }
        void setY(int y) { m_y = y; }
    };
    class SPImage;
    class SPTreeViewElement : public SPElement {
	protected:
		vector<SPTreeItem *> m_items;
		int itemHeight;
        static constexpr int kSectionHeight = 22;
        static constexpr int kIndent        = 20;
        static constexpr int kArrowWidth    = 16;
        static constexpr int kChildIndent   = 32;
        static constexpr int kIconSize      = 16; // icon rendered at 16×16
        static constexpr int kIconSlot      = 20; // 16px icon + 4px gap

        map<string, SPImage*> m_iconCache;

        // Returns all visible rows as (item, depth) pairs, respecting collapsed state
        [[nodiscard]] vector<pair<SPTreeItem*, int>> flattenVisible() const;
	public:
        void treeview_mousedown(SPObject* sender, SPEventArgs* e);
        void treeview_mouseup(SPObject* sender, SPEventArgs* e);
        explicit SPTreeViewElement(SPElement *parent);
        virtual string getType() override { return "treeview"; }
    	vector<SPTreeItem *>& getItems() { return m_items; }
		void paint(int x, int y, int width, int height, SPGraphicsContext *g) override;
		void setItemHeight(int height);
		int getItemHeight();
        void addItem(SPTreeItem *item);
        // Create and add a section header (purely visual label, not navigable)
        SPTreeItem* addSection(const string& title);
        // Remove all items (and their children), freeing memory
        void clearItems();
        // Traverse all items recursively (for selection sync)
        void forEachItem(const function<void(SPTreeItem*)>& fn) const;
	};
}
