#include "ui/LeftLibraryMenu.h"

#include "SPTreeItem.h"
#include "SPTreeViewElement.h"
#include "SPUri.h"

namespace space {
namespace ui {
namespace {

SPTreeItem* createItem(const navigation::SidebarNode& node) {
    auto* item = new SPTreeItem(new SPUri(node.getUri()));
    item->setText(node.getLabel());
    item->setIconPath(node.getIconPath());
    item->setIsBookmark(node.isBookmark());
    item->setCollapsed(!node.isExpanded());
    for (const auto& child : node.getChildren())
        item->addChild(createItem(child));
    return item;
}

} // namespace

LeftLibraryMenu::LeftLibraryMenu(SPTreeViewElement* tree) : m_tree(tree) {}

void LeftLibraryMenu::setNodes(const std::vector<navigation::SidebarNode>& nodes) {
    if (!m_tree) return;

    // Group by section; an empty label collects top-level items that are not
    // in any section.
    std::vector<std::pair<std::string, std::vector<const navigation::SidebarNode*>>> groups;
    auto groupFor = [&](const std::string& label) -> std::vector<const navigation::SidebarNode*>& {
        for (auto& group : groups)
            if (group.first == label) return group.second;
        groups.emplace_back(label, std::vector<const navigation::SidebarNode*>());
        return groups.back().second;
    };
    for (const auto& node : nodes) {
        if (node.isSection()) {
            auto& items = groupFor(node.getLabel());
            for (const auto& child : node.getChildren()) items.push_back(&child);
        } else {
            groupFor({}).push_back(&node);
        }
    }

    m_tree->clearItems();
    for (const auto& [label, items] : groups) {
        if (!label.empty()) m_tree->addSection(label);
        for (const auto* node : items) m_tree->addItem(createItem(*node));
    }
    select(m_selectedUri);
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

} // namespace ui
} // namespace space
