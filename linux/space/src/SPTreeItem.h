#pragma once
#include "SPObject.h"
#include <string>
#include <vector>
#include "SPUri.h"

#include <algorithm>
using namespace std;
namespace space {
	class SPUri;
    class SPTreeItem : SPObject {
    protected:
        bool highlighted = false;
		string title;
		string subtitle;
		string m_iconPath;
		SPUri *uri;
		bool selected = false;
        bool m_isSection = false;
        bool m_collapsed = true;
        bool m_isBookmark = false;
        vector<SPTreeItem*> m_children;
	public:
	    [[nodiscard]] bool isHighlighted() const { return this->highlighted; }
	    void setHighlighted(bool value) { this->highlighted = value; }
	    [[nodiscard]] bool isSelected() const { return this->selected; }
	    void setSelected(bool selected) { this->selected = selected; }

        // Section header — purely visual, never fires navigation events
        [[nodiscard]] bool isSection() const { return m_isSection; }
        void setIsSection(bool v) { m_isSection = v; }

        // Collapse/expand for items with children
        [[nodiscard]] bool isCollapsed() const { return m_collapsed; }
        void setCollapsed(bool v) { m_collapsed = v; }

        // Whether this item was created from a user bookmark (can be removed)
        [[nodiscard]] bool isBookmark() const { return m_isBookmark; }
        void setIsBookmark(bool v) { m_isBookmark = v; }

        // Children (one level deep)
        void addChild(SPTreeItem* child) { m_children.push_back(child); }
        void removeChild(SPTreeItem* child) {
            m_children.erase(remove(m_children.begin(), m_children.end(), child), m_children.end());
        }
        void clearChildren() {
            for (auto* c : m_children) delete c;
            m_children.clear();
        }
        [[nodiscard]] bool hasChildren() const { return !m_children.empty(); }
        [[nodiscard]] const vector<SPTreeItem*>& getChildren() const { return m_children; }

		[[nodiscard]] const string& getIconPath() const { return m_iconPath; }
		void setIconPath(const string& path) { m_iconPath = path; }

		SPTreeItem(); // for section headers — no URI
		explicit SPTreeItem(SPUri *uri);
		string getText() { return this->title; }
		void setText(string value) { this->title = value; }
		[[nodiscard]] SPUri *getUri() const { return this->uri; }
		~SPTreeItem() override {
            clearChildren();
            delete uri;
        }
	};
}
