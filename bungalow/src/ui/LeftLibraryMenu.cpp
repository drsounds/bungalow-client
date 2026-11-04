#include "ui/LeftLibraryMenu.h"

#include "SPTreeItem.h"
#include "SPTreeViewElement.h"
#include "SPMouseEventArgs.h"
#include "SPUri.h"
#include "BungalowApp.h"
#include "Database.h"
#include "app/library/MusicDragDrop.h"
#include "playlist/RootlistManager.h"

#include <limits>

namespace space {
namespace ui {

LeftLibraryMenu::LeftLibraryMenu(SPTreeViewElement* tree) : m_tree(tree) {
    views::MusicDragDrop::setHoverCallback([this] { onDragHoverDwell(); });
}

LeftLibraryMenu::~LeftLibraryMenu() {
    views::MusicDragDrop::setHoverCallback({});
    views::MusicDragDrop::clear();
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
    views::MusicDragDrop::clearHoverTarget();
    m_dropTarget = nullptr;

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

void LeftLibraryMenu::clearDropTarget() {
    if (m_dropTarget) {
        m_dropTarget->setDropTarget(false);
        m_dropTarget = nullptr;
        m_tree->invalidate();
    }
    views::MusicDragDrop::clearHoverTarget();
}

void LeftLibraryMenu::onDragHoverDwell() {
    if (!m_dropTarget || !m_tree) return;
    m_dropTarget->setDropTarget(false);
    auto* eventArgs = new TreeViewEventArgs();
    eventArgs->setItem(m_dropTarget);
    m_tree->invalidate();
    m_tree->notify("itemdraghovered", m_tree, eventArgs);
}

int LeftLibraryMenu::onDragMove(SPObject* /*sender*/, SPEventArgs* eventArgs) {
    auto* args = dynamic_cast<SPMouseEventArgs*>(eventArgs);
    if (!args || !views::MusicDragDrop::active()) {
        clearDropTarget();
        return 0;
    }
    if (views::MusicDragDrop::released()) {
        clearDropTarget();
        views::MusicDragDrop::clear();
        return 0;
    }
    if (m_dropTarget && !m_dropTarget->isDropTarget() &&
        views::MusicDragDrop::hoverUri().empty())
        clearDropTarget();

    views::MusicDragDrop::checkHoverDwell();

    SPTreeItem* hovered = m_tree->itemAtAbsoluteY(args->getY());
    const std::string uri = hovered && hovered->getUri()
        ? hovered->getUri()->getUri()
        : std::string();
    if (!hovered || !hovered->isDraggable() || playlist::RootlistManager::playlistId(uri).empty())
        hovered = nullptr;
    if (hovered != m_dropTarget) {
        clearDropTarget();
        m_dropTarget = hovered;
        if (m_dropTarget) {
            m_dropTarget->setDropTarget(true);
            views::MusicDragDrop::setHoverTarget(uri);
        }
        m_tree->invalidate();
    }
    return 0;
}

int LeftLibraryMenu::onDragEnd(SPObject* /*sender*/, SPEventArgs* eventArgs) {
    auto* args = dynamic_cast<SPMouseEventArgs*>(eventArgs);
    if (!args || !views::MusicDragDrop::active()) {
        clearDropTarget();
        return 0;
    }
    SPTreeItem* item = m_tree->itemAtAbsoluteY(args->getY());
    const std::string uri = item && item->getUri()
        ? item->getUri()->getUri()
        : std::string();
    const std::string id = playlist::RootlistManager::playlistId(uri);
    clearDropTarget();
    if (!id.empty() && item->isDraggable()) {
        auto* app = BungalowApp::current();
        Database* db = app ? app->getDatabase() : nullptr;
        if (!db || !playlist::RootlistManager::insertContentRefs(
                *db, uri, views::MusicDragDrop::refs(),
                std::numeric_limits<int>::max())) {
            if (app) app->showError("Could not add items to this playlist.");
        }
    }
    views::MusicDragDrop::clear();
    return 0;
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
