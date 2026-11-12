#include "ui/LeftLibraryMenu.h"

#include "SPTreeItem.h"
#include "SPTreeViewElement.h"
#include "SPUri.h"
#include "BungalowApp.h"
#include "Database.h"
#include "playlist/RootlistManager.h"

#include <limits>

namespace space {
namespace ui {

LeftLibraryMenu::LeftLibraryMenu(SPTreeViewElement* tree) : m_tree(tree) {
    SPDragDrop::registerTarget(m_tree, this);
}

LeftLibraryMenu::~LeftLibraryMenu() {
    SPDragDrop::unregisterTarget(this);
}

SPTreeItem* LeftLibraryMenu::createItem(const navigation::SidebarNode& node) {
    auto* item = new SPTreeItem(new SPUri(node.getUri()));
    item->setText(node.getLabel());
    item->setIconPath(node.getIconPath());
    item->setIsBookmark(node.isBookmark());
    item->setCollapsed(!node.isExpanded());
    item->setDraggable(static_cast<bool>(node.getOnMove()));
    item->setEditOnClick(node.isEditOnClick() && node.getOnRename());
    if (node.getOnRename() || node.getOnMove() || node.getOnRemove())
        m_handlers[item] = {node.getOnRename(), node.getOnMove(), node.getOnRemove()};
    for (const auto& child : node.getChildren())
        item->addChild(createItem(child));
    return item;
}

void LeftLibraryMenu::setNodes(const std::vector<navigation::SidebarNode>& nodes) {
    if (!m_tree) return;
    m_dropTarget = nullptr; // the items are about to be replaced

    // Group by section; an empty label collects top-level items that are not
    // in any section.
    struct Group {
        std::string label;
        bool spacer = false;
        std::vector<const navigation::SidebarNode*> items;
    };
    std::vector<Group> groups;
    auto groupFor = [&](const std::string& label, bool spacer) -> std::vector<const navigation::SidebarNode*>& {
        for (auto& group : groups)
            if (group.label == label) return group.items;
        groups.push_back({label, spacer, {}});
        return groups.back().items;
    };
    for (const auto& node : nodes) {
        if (node.isSection()) {
            auto& items = groupFor(node.getLabel(), node.isSpacer());
            for (const auto& child : node.getChildren()) items.push_back(&child);
        } else {
            groupFor({}, false).push_back(&node);
        }
    }

    m_tree->clearItems();
    m_handlers.clear();
    for (const auto& group : groups) {
        if (group.spacer) m_tree->addSpacer();
        else if (!group.label.empty()) m_tree->addSection(group.label);
        for (const auto* node : group.items) m_tree->addItem(createItem(*node));
    }
    select(m_selectedUri);
}

// ── Drop target ───────────────────────────────────────────────────────────────

SPTreeItem* LeftLibraryMenu::playlistItemAt(int y) const {
    SPTreeItem* item = m_tree ? m_tree->itemAtAbsoluteY(y) : nullptr;
    if (!item || !item->isDraggable() || !item->getUri()) return nullptr;
    return playlist::RootlistManager::playlistId(item->getUri()->getUri()).empty() ? nullptr : item;
}

void LeftLibraryMenu::setDropTarget(SPTreeItem* item) {
    if (item == m_dropTarget) return;
    if (m_dropTarget) m_dropTarget->setDropTarget(false);
    m_dropTarget = item;
    if (m_dropTarget) {
        m_dropTarget->setDropTarget(true);
        m_hoverStarted = std::chrono::steady_clock::now();
        m_hoverOpened = false;
    }
    if (m_tree) m_tree->invalidate();
}

bool LeftLibraryMenu::dragOver(int /*x*/, int y, const SPDragData& /*data*/) {
    setDropTarget(playlistItemAt(y));
    if (!m_dropTarget) return false;

    // Resting on a playlist opens it. The item stays the drop target.
    if (!m_hoverOpened && std::chrono::steady_clock::now() - m_hoverStarted >= kHoverOpenDelay) {
        m_hoverOpened = true;
        TreeViewEventArgs eventArgs;
        eventArgs.setItem(m_dropTarget);
        m_tree->notify("itemdraghovered", m_tree, &eventArgs);
    }
    return m_dropTarget != nullptr;
}

void LeftLibraryMenu::dragLeave() {
    setDropTarget(nullptr);
}

bool LeftLibraryMenu::drop(int /*x*/, int y, const SPDragData& data) {
    SPTreeItem* item = playlistItemAt(y);
    setDropTarget(nullptr);
    if (!item) return false;
    // Copied: adding to the playlist may rebuild the sidebar's items.
    const std::string uri = item->getUri()->getUri();

    auto* app = BungalowApp::current();
    Database* db = app ? app->getDatabase() : nullptr;
    if (!db || !playlist::RootlistManager::insertContentRefs(
            *db, uri, data.uris, std::numeric_limits<int>::max())) {
        if (app) app->showError("Could not add items to this playlist.");
        return false;
    }
    return true;
}

void LeftLibraryMenu::select(const std::string& uri) {
    m_selectedUri = uri;
    if (!m_tree) return;
    m_tree->forEachItem([&uri](SPTreeItem* item) {
        if (item->getUri() && !item->getUri()->getUri().empty())
            item->setSelected(item->getUri()->getUri() == uri);
    });
    m_tree->invalidate();
}

const LeftLibraryMenu::Handlers* LeftLibraryMenu::handlersFor(SPTreeItem* item) const {
    auto found = m_handlers.find(item);
    return found == m_handlers.end() ? nullptr : &found->second;
}

bool LeftLibraryMenu::canRename(SPTreeItem* item) const {
    const Handlers* handlers = handlersFor(item);
    return handlers && handlers->onRename && item && !item->isEditOnClick();
}

bool LeftLibraryMenu::canRemove(SPTreeItem* item) const {
    const Handlers* handlers = handlersFor(item);
    return handlers && handlers->onRemove;
}

void LeftLibraryMenu::beginRename(SPTreeItem* item) {
    if (m_tree && canRename(item)) m_tree->beginEdit(item, item->getText());
}

// The handlers usually change what the sidebar shows, which rebuilds the
// items and their handlers, so each is copied before it is called.

void LeftLibraryMenu::rename(SPTreeItem* item, const std::string& text) {
    const Handlers* handlers = handlersFor(item);
    if (!handlers || !handlers->onRename) return;
    const auto handler = handlers->onRename;
    handler(text);
}

void LeftLibraryMenu::move(SPTreeItem* item, int index) {
    const Handlers* handlers = handlersFor(item);
    if (!handlers || !handlers->onMove) return;
    const auto handler = handlers->onMove;
    handler(index);
}

void LeftLibraryMenu::remove(SPTreeItem* item) {
    const Handlers* handlers = handlersFor(item);
    if (!handlers || !handlers->onRemove) return;
    const auto handler = handlers->onRemove;
    handler();
}

} // namespace ui
} // namespace space
