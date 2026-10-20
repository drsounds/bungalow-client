#include "SPTreeViewElement.h"
#include "SPMouseEventArgs.h"
#include "SPWindowElement.h"
#include "SPTreeItem.h"
#include "SPSkin.h"
#include "SPGraphicsContext.h"
#include "SPImage.h"
#include "SPColor.h"
#include "SPUri.h"
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

// ── Mouse handlers ────────────────────────────────────────────────────────────

void SPTreeViewElement::treeview_mousedown(SPObject */*sender*/, SPEventArgs *e) {
    SPMouseEventArgs *args = (SPMouseEventArgs *)e;
    SPTreeViewElement *treeView = this;

    // Right-click mousedown: do nothing here, handle on mouseup
    if (args->getButton() == 1) return;

    int relY = args->getY() - treeView->getAbsoluteBounds().y;
    int top = 0;

    auto rows = treeView->flattenVisible();
    for (auto& [item, depth] : rows) {
        int rowH = item->isSection() ? kSectionHeight : treeView->getItemHeight();
        if (relY > top && relY < top + rowH) {
            if (!item->isSection()) {
                item->setHighlighted(true);
                TreeViewEventArgs *eventArgs = new TreeViewEventArgs();
                eventArgs->setItem(item);
                treeView->notify(string("itemhighlighted"), treeView, eventArgs);
            }
        } else {
            item->setHighlighted(false);
        }
        top += rowH;
    }

    sp_rectangle region;
    region.x = treeView->getAbsoluteBounds().x;
    region.y = treeView->getAbsoluteBounds().y;
    region.z = 0;
    region.width  = treeView->getWidth();
    region.height = treeView->getHeight();
    treeView->getWindowElement()->invalidateRegion(region);
}

void SPTreeViewElement::treeview_mouseup(SPObject *sender, SPEventArgs *e) {
    SPMouseEventArgs *args = (SPMouseEventArgs *)e;
    SPTreeViewElement *treeView = this;

    int relX = args->getX() - treeView->getAbsoluteBounds().x;
    int relY = args->getY() - treeView->getAbsoluteBounds().y;

    // Clear all highlights
    treeView->forEachItem([](SPTreeItem* item) { item->setHighlighted(false); });

    auto rows = treeView->flattenVisible();
    int top = 0;
    SPTreeItem *clickedItem = nullptr;
    int clickedDepth = 0;

    for (auto& [item, depth] : rows) {
        int rowH = item->isSection() ? kSectionHeight : treeView->getItemHeight();
        if (relY > top && relY < top + rowH) {
            clickedItem = item;
            clickedDepth = depth;
        }
        top += rowH;
    }

    if (clickedItem != nullptr && !clickedItem->isSection()) {
        bool isRightClick = (args->getButton() == 1);

        if (isRightClick) {
            // Fire context menu event (only for bookmark items)
            if (clickedItem->isBookmark()) {
                TreeViewEventArgs *eventArgs = new TreeViewEventArgs();
                eventArgs->setItem(clickedItem);
                eventArgs->setX(args->getX());
                eventArgs->setY(args->getY());
                treeView->notify(string("itemcontextmenu"), treeView, eventArgs);
            }
        } else {
            // Left click: check if user clicked the arrow area for expandable items
            int textIndent = (clickedDepth == 1) ? kChildIndent : kIndent;
            bool clickedArrow = clickedItem->hasChildren() && (relX < textIndent);

            if (clickedArrow) {
                // Toggle collapse — no navigation event
                clickedItem->setCollapsed(!clickedItem->isCollapsed());
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

    sp_rectangle region;
    region.x = treeView->getAbsoluteBounds().x;
    region.y = treeView->getAbsoluteBounds().y;
    region.z = 0;
    region.width  = treeView->getWidth();
    region.height = treeView->getHeight();
    treeView->getWindowElement()->invalidateRegion(region);
}

// ── Construction ──────────────────────────────────────────────────────────────

SPTreeViewElement::SPTreeViewElement(SPElement *parent)
: SPElement(parent) {
	this->setTagName("treeview");
	this->setParentNode(parent);

	this->itemHeight = 24;
	this->addEventListener(string("mousedown"), SPCallback(&SPTreeViewElement::treeview_mousedown), this, this);
	this->addEventListener(string("mouseup"),   SPCallback(&SPTreeViewElement::treeview_mouseup),   this, this);
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

void SPTreeViewElement::clearItems() {
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
    int top = 0;

    for (auto& [item, depth] : flattenVisible()) {
        int rowH = item->isSection() ? kSectionHeight : itemHeight;

        if (item->isSection()) {
            this->getSkin()->drawSectionHeader(g, this, item->getText(), x, y + top, width, rowH);
        } else {
            // Selection / highlight background
            this->getSkin()->drawTreeItem(g, this, item, rowH, x, y + top, width, rowH);

            // Disclosure arrow for expandable items
            if (item->hasChildren()) {
                string arrow = item->isCollapsed() ? "\xe2\x96\xb6" : "\xe2\x96\xbc"; // ▶ / ▼
                SPColor arrowColor(128, 128, 128, 180);
                g->drawString(arrow, fs, arrowColor, x + 4, y + top, kArrowWidth, rowH);
            }

            // Icon — always reserve kIconSlot px regardless of whether icon loads
            int iconX = x + (depth == 1 ? kChildIndent : kIndent);
            const string& iconPath = item->getIconPath();
            if (!iconPath.empty()) {
                auto [it, inserted] = m_iconCache.emplace(iconPath, nullptr);
                if (inserted)
                    it->second = g->loadImage(iconPath); // nullptr if load failed — don't retry
                if (it->second != nullptr) {
                    int iconY = y + top + (rowH - kIconSize) / 2;
                    g->drawImage(it->second, iconX, iconY, kIconSize, kIconSize);
                }
            }

            // Item text — shifted right to make room for the icon slot
            int textX = iconX + kIconSlot;
            sp_rectangle strB = g->measureString(item->getText(), fs);
            int ttop = top + (rowH / 2) - (strB.height / 2);
            SPColor itemColor = item->isSelected() ? SPColor(95, 132, 180, 255) : color;
            g->drawString(item->getText(), fs, itemColor, textX, y + ttop, width, rowH);
        }
        top += rowH;
    }
}

} // namespace space
