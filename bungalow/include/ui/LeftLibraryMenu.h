#pragma once

#include "navigation/SidebarNode.h"

#include <string>
#include <vector>

namespace space {
class SPTreeViewElement;

namespace ui {

/// The sidebar: shows SidebarNodes in a tree view and keeps the item of the
/// current URI selected.
class LeftLibraryMenu {
public:
    explicit LeftLibraryMenu(SPTreeViewElement* tree);

    /// Replaces the tree's items. Top-level nodes without a URI become section
    /// headers followed by their children; sections with the same label are
    /// merged, in the order they first appear.
    void setNodes(const std::vector<navigation::SidebarNode>& nodes);

    /// Selects every item whose URI equals uri.
    void select(const std::string& uri);

    SPTreeViewElement* getTreeView() const { return m_tree; }

private:
    SPTreeViewElement* m_tree;
    std::string m_selectedUri;
};

} // namespace ui
} // namespace space
