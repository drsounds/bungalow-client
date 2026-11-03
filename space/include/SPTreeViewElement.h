#ifndef TREEVIEWELEMENT_H
#define TREEVIEWELEMENT_H
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
        string m_text;
        int m_index = -1;
    public:
        SPTreeItem *getItem() { return this->item; }
        void setItem(SPTreeItem *item) { this->item = item; }
        int getX() const { return m_x; }
        int getY() const { return m_y; }
        void setX(int x) { m_x = x; }
        void setY(int y) { m_y = y; }
        /// "itemrenamed": the text entered in the inline input.
        const string& getText() const { return m_text; }
        void setText(const string& text) { m_text = text; }
        /// "itemmoved": the item's new index among its adjacent draggable items.
        int getIndex() const { return m_index; }
        void setIndex(int index) { m_index = index; }
    };
    class SPImage;
    class SPInputElement;

    /// Sidebar tree. Fires "itemselected", "itemhighlighted",
    /// "itemcontextmenu" (right click), "itemrenamed" (inline input
    /// submitted, see beginEdit()) and "itemmoved" (a draggable item was
    /// dropped at a new position).
    class SPTreeViewElement : public SPElement {
	protected:
		vector<SPTreeItem *> m_items;
		int itemHeight;
        static constexpr int kSectionHeight = 22;
        static constexpr int kSpacerHeight  = 14;
        static constexpr int kIndent        = 20;
        static constexpr int kArrowWidth    = 16;
        static constexpr int kChildIndent   = 32;
        static constexpr int kIconSize      = 16; // icon rendered at 16×16
        static constexpr int kIconSlot      = 20; // 16px icon + 4px gap
        static constexpr int kDragThreshold = 4;  // px the pointer moves before a press becomes a drag
        static constexpr int kDragScrollZone = 16; // px from the edge that scrolls while dragging

        map<string, SPImage*> m_iconCache;

        // Drag and drop: the draggable item pressed with the left button,
        // which becomes a drag once the pointer moves far enough.
        SPTreeItem *m_pressedItem = nullptr;
        int m_pressY = 0;
        bool m_dragging = false;
        // Insertion slot in the dragged item's draggable run, -1 if none.
        int m_dropSlot = -1;

        // Inline editing
        SPInputElement *m_editInput = nullptr;
        SPTreeItem *m_editingItem = nullptr;

        // Returns all visible rows as (item, depth) pairs, respecting collapsed state
        [[nodiscard]] vector<pair<SPTreeItem*, int>> flattenVisible() const;
        [[nodiscard]] int rowHeight(const SPTreeItem *item) const;
        // The visible row at a y offset from the top of the content, or nullptr.
        SPTreeItem *itemAt(int contentY, int *depth = nullptr) const;
        // Offset of item's row from the top of the content, or -1 if not visible.
        [[nodiscard]] int itemTop(const SPTreeItem *item) const;
        // The top-level draggable items adjacent to item, item included.
        [[nodiscard]] vector<SPTreeItem*> draggableRun(const SPTreeItem *item) const;
        [[nodiscard]] int dropSlotAt(int contentY) const;
        // Width the rows take up, leaving room for the scrollbar.
        int getItemWidth();
        void endDrag();
        void layoutEditInput();
        void onScrolled() override;
	public:
        void treeview_mousedown(SPObject* sender, SPEventArgs* e);
        void treeview_mouseup(SPObject* sender, SPEventArgs* e);
        int treeview_mousemove(SPObject* sender, SPEventArgs* e);
        int onEditEnter(SPObject* sender, SPEventArgs* e);
        int onEditEscape(SPObject* sender, SPEventArgs* e);
        explicit SPTreeViewElement(SPElement *parent);
        virtual string getType() override { return "treeview"; }
    	vector<SPTreeItem *>& getItems() { return m_items; }
		void paint(int x, int y, int width, int height, SPGraphicsContext *g) override;
        int getContentHeight() override;
		void setItemHeight(int height);
		int getItemHeight();
        void addItem(SPTreeItem *item);
        // Create and add a section header (purely visual label, not navigable)
        SPTreeItem* addSection(const string& title);
        // A blank gap separating the items before and after it
        SPTreeItem* addSpacer();
        // Replaces item's label with a text input holding text. Enter fires
        // "itemrenamed" with the entered text; Escape, clicking elsewhere in
        // the tree or scrolling cancels.
        void beginEdit(SPTreeItem *item, const string& text);
        void endEdit();
        [[nodiscard]] bool isEditing() const { return m_editingItem != nullptr; }
        // Remove all items (and their children), freeing memory
        void clearItems();
        // Traverse all items recursively (for selection sync)
        void forEachItem(const function<void(SPTreeItem*)>& fn) const;
	};
}
#endif // TREEVIEWELEMENT_H
