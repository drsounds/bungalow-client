#include "SPTreeViewElement.h"
#include "SPMouseEventArgs.h"
#include "SPWindowElement.h"
#include "SPTreeItem.h"
#include "SPSkin.h"
#include "SPGraphicsContext.h"
#include "SPImage.h"
#include "SPColor.h"
#include "SPInputElement.h"
#include "SPInputEventArgs.h"
#include "SPUri.h"
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>

using namespace std;

namespace space {

// ── Helpers ───────────────────────────────────────────────────────────────────

vector<pair<SPTreeItem*, int>> SPTreeViewElement::flattenVisible() const {
    vector<pair<SPTreeItem*, int>> result;
    for (auto* item : m_items) {
        result.push_back({item, 0});
        if (!item->isSection() && item->hasChildren() && !item->isCollapsed()) {
            for (auto* child : item->getChildren()) {
                result.push_back({child, 1});
            }
        }
    }
    return result;
}

void SPTreeViewElement::forEachItem(const function<void(SPTreeItem*)>& fn) const {
    for (auto* item : m_items) {
        fn(item);
        for (auto* child : item->getChildren())
            fn(child);
    }
}

int SPTreeViewElement::rowHeight(const SPTreeItem *item) const {
    if (item->isSpacer()) return kSpacerHeight;
    return item->isSection() ? kSectionHeight : itemHeight;
}

SPTreeItem *SPTreeViewElement::itemAt(int contentY, int *depth) const {
    int top = 0;
    for (auto& [item, itemDepth] : flattenVisible()) {
        const int rowH = rowHeight(item);
        if (contentY >= top && contentY < top + rowH) {
            if (depth) *depth = itemDepth;
            return item;
        }
        top += rowH;
    }
    return nullptr;
}

int SPTreeViewElement::itemTop(const SPTreeItem *item) const {
    int top = 0;
    for (auto& [row, depth] : flattenVisible()) {
        if (row == item) return top;
        top += rowHeight(row);
    }
    return -1;
}

vector<SPTreeItem*> SPTreeViewElement::draggableRun(const SPTreeItem *item) const {
    vector<SPTreeItem*> run;
    for (auto* candidate : m_items) {
        if (!candidate->isDraggable()) {
            // A gap ends the run, unless it is the one holding item.
            if (find(run.begin(), run.end(), item) != run.end()) break;
            run.clear();
            continue;
        }
        run.push_back(candidate);
    }
    if (find(run.begin(), run.end(), item) == run.end()) run.clear();
    return run;
}

int SPTreeViewElement::dropSlotAt(int contentY) const {
    const vector<SPTreeItem*> run = draggableRun(m_pressedItem);
    for (size_t i = 0; i < run.size(); ++i) {
        if (contentY < itemTop(run[i]) + rowHeight(run[i]) / 2)
            return static_cast<int>(i);
    }
    return static_cast<int>(run.size());
}

int SPTreeViewElement::getItemWidth() {
    return max(0, this->getWidth() - this->getVerticalScrollBarWidth());
}

int SPTreeViewElement::getContentHeight() {
    int height = 0;
    for (auto& [item, depth] : flattenVisible())
        height += rowHeight(item);
    return height;
}

// ── Mouse handlers ────────────────────────────────────────────────────────────

void SPTreeViewElement::treeview_mousedown(SPObject */*sender*/, SPEventArgs *e) {
    SPMouseEventArgs *args = (SPMouseEventArgs *)e;
    SPTreeViewElement *treeView = this;

    // The inline input is a native control, so this click is somewhere else.
    if (isEditing()) endEdit();

    // Right-click mousedown: do nothing here, handle on mouseup
    if (args->getButton() == 1) return;
    endDrag();

    const int contentY = args->getY() - treeView->getAbsoluteBounds().y + this->getScrollY();
    SPTreeItem *pressed = itemAt(contentY);

    for (auto& [item, depth] : treeView->flattenVisible()) {
        if (item == pressed && !item->isSection()) {
            item->setHighlighted(true);
            TreeViewEventArgs *eventArgs = new TreeViewEventArgs();
            eventArgs->setItem(item);
            treeView->notify(string("itemhighlighted"), treeView, eventArgs);
        } else {
            item->setHighlighted(false);
        }
    }

    if (pressed != nullptr && pressed->isDraggable()) {
        m_pressedItem = pressed;
        m_pressY = args->getY();
        // Keep the moves and the release even if the pointer leaves the tree.
        SPElement::setMouseCapture(this);
    }

    treeView->invalidate();
}

int SPTreeViewElement::treeview_mousemove(SPObject */*sender*/, SPEventArgs *e) {
    auto *args = dynamic_cast<SPMouseEventArgs *>(e);
    if (!args || !m_pressedItem) return 0;

    if (!m_dragging) {
        if (abs(args->getY() - m_pressY) < kDragThreshold) return 0;
        m_dragging = true;
    }

    // Scroll while the pointer is held near (or past) the top or bottom edge.
    const sp_rectangle b = this->getAbsoluteBounds();
    if (args->getY() < b.y + kDragScrollZone)
        this->scrollToY(this->getScrollY() - kDragScrollZone / 2);
    else if (args->getY() > b.y + b.height - kDragScrollZone)
        this->scrollToY(this->getScrollY() + kDragScrollZone / 2);

    m_dropSlot = dropSlotAt(args->getY() - b.y + this->getScrollY());
    this->invalidate();
    return 0;
}

void SPTreeViewElement::endDrag() {
    if (m_pressedItem != nullptr && SPElement::s_capturedElement == this)
        SPElement::releaseMouseCapture();
    m_pressedItem = nullptr;
    m_dragging = false;
    m_dropSlot = -1;
}

void SPTreeViewElement::treeview_mouseup(SPObject *sender, SPEventArgs *e) {
    SPMouseEventArgs *args = (SPMouseEventArgs *)e;
    SPTreeViewElement *treeView = this;

    int relX = args->getX() - treeView->getAbsoluteBounds().x;
    int relY = args->getY() - treeView->getAbsoluteBounds().y + this->getScrollY();

    // Clear all highlights
    treeView->forEachItem([](SPTreeItem* item) { item->setHighlighted(false); });

    if (m_dragging) {
        SPTreeItem *dragged = m_pressedItem;
        const vector<SPTreeItem*> run = draggableRun(dragged);
        const int slot = m_dropSlot;
        endDrag();
        const auto found = find(run.begin(), run.end(), dragged);
        if (found != run.end() && slot >= 0) {
            const int oldIndex = static_cast<int>(found - run.begin());
            // Taking the item out shifts the slots after it up by one.
            const int newIndex = slot > oldIndex ? slot - 1 : slot;
            if (newIndex != oldIndex) {
                auto *eventArgs = new TreeViewEventArgs();
                eventArgs->setItem(dragged);
                eventArgs->setIndex(newIndex);
                treeView->notify(string("itemmoved"), treeView, eventArgs);
            }
        }
        treeView->invalidate();
        return;
    }
    endDrag();

    int clickedDepth = 0;
    SPTreeItem *clickedItem = itemAt(relY, &clickedDepth);

    if (clickedItem != nullptr && !clickedItem->isSection()) {
        bool isRightClick = (args->getButton() == 1);

        if (isRightClick) {
            TreeViewEventArgs *eventArgs = new TreeViewEventArgs();
            eventArgs->setItem(clickedItem);
            eventArgs->setX(args->getX());
            eventArgs->setY(args->getY());
            treeView->notify(string("itemcontextmenu"), treeView, eventArgs);
        } else {
            // Left click: check if user clicked the arrow area for expandable items
            int textIndent = (clickedDepth == 1) ? kChildIndent : kIndent;
            bool clickedArrow = clickedItem->hasChildren() && (relX < textIndent);

            if (clickedArrow) {
                // Toggle collapse — no navigation event
                clickedItem->setCollapsed(!clickedItem->isCollapsed());
            } else if (clickedItem->isEditOnClick()) {
                beginEdit(clickedItem, "");
            } else {
                // Navigate: deselect all, select this item, fire event
                treeView->forEachItem([](SPTreeItem* item) { item->setSelected(false); });
                clickedItem->setSelected(true);
                TreeViewEventArgs *eventArgs = new TreeViewEventArgs();
                eventArgs->setItem(clickedItem);
                treeView->notify(string("itemselected"), treeView, eventArgs);
            }
        }
    }

    treeView->invalidate();
}

// ── Inline editing ────────────────────────────────────────────────────────────

void SPTreeViewElement::beginEdit(SPTreeItem *item, const string& text) {
    if (item == nullptr || itemTop(item) < 0) return;
    endEdit();

    // Bring the row into view first: scrolling cancels editing.
    const int top = itemTop(item);
    if (top < this->getScrollY())
        this->scrollToY(top);
    else if (top + rowHeight(item) > this->getScrollY() + this->getHeight())
        this->scrollToY(top + rowHeight(item) - this->getHeight());

    if (m_editInput == nullptr) {
        m_editInput = new SPInputElement(this);
        // Backends key native controls by element id.
        m_editInput->setId(this->getId() + "-edit");
        m_editInput->addEventListener("enter",  SPCallback(&SPTreeViewElement::onEditEnter),  this, m_editInput);
        m_editInput->addEventListener("escape", SPCallback(&SPTreeViewElement::onEditEscape), this, m_editInput);
        this->appendChild(m_editInput);
    }
    m_editingItem = item;
    layoutEditInput();
    m_editInput->setValue(text);
    m_editInput->setVisible(true);
    m_editInput->focus();
    this->invalidate();
}

void SPTreeViewElement::endEdit() {
    m_editingItem = nullptr;
    if (m_editInput != nullptr && m_editInput->isVisible())
        m_editInput->setVisible(false);
    this->invalidate();
}

void SPTreeViewElement::layoutEditInput() {
    if (m_editInput == nullptr || m_editingItem == nullptr) return;
    int depth = 0;
    for (auto& [item, itemDepth] : flattenVisible())
        if (item == m_editingItem) depth = itemDepth;
    const int top = itemTop(m_editingItem);
    const int rowH = rowHeight(m_editingItem);
    // Over the label, next to the icon.
    const int left = (depth == 1 ? kChildIndent : kIndent) + kIconSlot - 4;
    m_editInput->setX(left);
    m_editInput->setY(top + 2);
    m_editInput->setWidth(getItemWidth() - left - 8);
    m_editInput->setHeight(rowH - 4);
}

int SPTreeViewElement::onEditEnter(SPObject */*sender*/, SPEventArgs *e) {
    auto *inputArgs = dynamic_cast<SPInputEventArgs *>(e);
    SPTreeItem *item = m_editingItem;
    string text = inputArgs ? inputArgs->getText() : string();
    endEdit();

    const auto first = text.find_first_not_of(" \t\r\n");
    const auto last = text.find_last_not_of(" \t\r\n");
    text = first == string::npos ? string() : text.substr(first, last - first + 1);
    if (item == nullptr || text.empty()) return 0;

    auto *eventArgs = new TreeViewEventArgs();
    eventArgs->setItem(item);
    eventArgs->setText(text);
    this->notify(string("itemrenamed"), this, eventArgs);
    return 0;
}

int SPTreeViewElement::onEditEscape(SPObject */*sender*/, SPEventArgs */*e*/) {
    endEdit();
    return 0;
}

void SPTreeViewElement::onScrolled() {
    if (isEditing()) endEdit();
}

// ── Construction ──────────────────────────────────────────────────────────────

SPTreeViewElement::SPTreeViewElement(SPElement *parent)
: SPElement(parent) {
	this->setTagName("treeview");
	this->setParentNode(parent);
	this->setOverflowY(SPOverflow::Auto);

	this->itemHeight = 24;
	this->addEventListener(string("mousedown"), SPCallback(&SPTreeViewElement::treeview_mousedown), this, this);
	this->addEventListener(string("mouseup"),   SPCallback(&SPTreeViewElement::treeview_mouseup),   this, this);
	this->addEventListener(string("mousemove"), SPCallback(&SPTreeViewElement::treeview_mousemove), this, this);
}

void SPTreeViewElement::addItem(SPTreeItem *item) {
    this->m_items.push_back(item);
}

SPTreeItem* SPTreeViewElement::addSection(const string& title) {
    auto* item = new SPTreeItem(); // no URI — section headers are not navigable
    item->setText(title);
    item->setIsSection(true);
    item->setCollapsed(false);
    m_items.push_back(item);
    return item;
}

SPTreeItem* SPTreeViewElement::addSpacer() {
    SPTreeItem* item = addSection("");
    item->setIsSpacer(true);
    return item;
}

void SPTreeViewElement::clearItems() {
    // The items go away, so any edit or drag of them ends.
    endEdit();
    endDrag();
    m_items.clear();
    m_iconCache.clear();
}

void SPTreeViewElement::setItemHeight(int height) { this->itemHeight = height; }
int  SPTreeViewElement::getItemHeight()            { return this->itemHeight; }

// ── Paint ─────────────────────────────────────────────────────────────────────

void SPTreeViewElement::paint(int x, int y, int width, int height, SPGraphicsContext *g)
{
    // Background
    SPColor bgColor = this->getBackgroundColor();
    if (auto tint = this->getTintColor(); tint)
        bgColor = bgColor.tint(*tint);
    if (!this->getSkin()->drawElement(g, this, "", sp_rectangle { x, y, width, height, 0 }))
        g->fillRectangle(x, y, width, height, bgColor);

    SPColor color    = this->getForegroundColor();
    SPFontStyle fs   = this->getFontStyle();
    const int itemWidth = getItemWidth();
    // Rows are laid out from the top of the content, which scrolls.
    const int contentTop = y - this->getScrollY();
    int top = 0;

    layoutEditInput();

    for (auto& [item, depth] : flattenVisible()) {
        int rowH = rowHeight(item);
        const int rowY = contentTop + top;
        top += rowH;
        if (rowY + rowH <= y || rowY >= y + height || item->isSpacer())
            continue;

        if (item->isSection()) {
            this->getSkin()->drawSectionHeader(g, this, item->getText(), x, rowY, itemWidth, rowH);
        } else {
            // Selection / highlight background
            this->getSkin()->drawTreeItem(g, this, item, rowH, x, rowY, itemWidth, rowH);

            // Disclosure arrow for expandable items
            if (item->hasChildren()) {
                string arrow = item->isCollapsed() ? "\xe2\x96\xb6" : "\xe2\x96\xbc"; // ▶ / ▼
                SPColor arrowColor(128, 128, 128, 180);
                g->drawString(arrow, fs, arrowColor, x + 4, rowY, kArrowWidth, rowH);
            }

            // Icon — always reserve kIconSlot px regardless of whether icon loads
            int iconX = x + (depth == 1 ? kChildIndent : kIndent);
            const string& iconPath = item->getIconPath();
            if (!iconPath.empty()) {
                auto [it, inserted] = m_iconCache.emplace(iconPath, nullptr);
                if (inserted)
                    it->second = g->loadImage(iconPath); // nullptr if load failed — don't retry
                if (it->second != nullptr) {
                    int iconY = rowY + (rowH - kIconSize) / 2;
                    g->drawImage(it->second, iconX, iconY, kIconSize, kIconSize);
                }
            }

            // Item text — shifted right to make room for the icon slot. The
            // inline input takes its place while the item is being edited.
            if (item != m_editingItem) {
                int textX = iconX + kIconSlot;
                sp_rectangle strB = g->measureString(item->getText(), fs);
                int ttop = rowY + (rowH / 2) - (strB.height / 2);
                SPColor itemColor = item->isSelected() ? SPColor(95, 132, 180, 255) : color;
                g->drawString(item->getText(), fs, itemColor, textX, ttop, itemWidth, rowH);
            }
        }
    }

    // Where a dragged item would land.
    if (m_dragging && m_dropSlot >= 0) {
        const vector<SPTreeItem*> run = draggableRun(m_pressedItem);
        if (!run.empty()) {
            const int lineTop = m_dropSlot < static_cast<int>(run.size())
                ? itemTop(run[m_dropSlot])
                : itemTop(run.back()) + rowHeight(run.back());
            g->fillRectangle(x + 4, contentTop + lineTop - 1, max(0, itemWidth - 8), 2,
                             SPColor(99, 149, 234, 255));
        }
    }
}

} // namespace space
