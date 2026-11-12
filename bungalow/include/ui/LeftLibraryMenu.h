#pragma once

#include "navigation/SidebarNode.h"

#include <map>
#include <string>
#include <vector>

namespace space {
class SPObject;
class SPEventArgs;
class SPTreeItem;
class SPTreeViewElement;

namespace ui {

/// The sidebar: shows SidebarNodes in a tree view and keeps the item of the
/// current URI selected.
class LeftLibraryMenu {
public:
    explicit LeftLibraryMenu(SPTreeViewElement* tree);
    ~LeftLibraryMenu();

    /// Replaces the tree's items. Top-level nodes without a URI become section
    /// headers (or a blank gap, for spacer sections) followed by their
    /// children; sections with the same label are merged, in the order they
    /// first appear.
    void setNodes(const std::vector<navigation::SidebarNode>& nodes);

    /// Selects every item whose URI equals uri.
    void select(const std::string& uri);

    /// What the node shown as item supports; see SidebarNode.
    bool canRename(SPTreeItem* item) const;
    bool canRemove(SPTreeItem* item) const;

    /// Shows an inline input over item, holding its current label.
    void beginRename(SPTreeItem* item);

    /// Hand the tree's "itemrenamed" and "itemmoved" events, and a delete
    /// chosen from the context menu, to the node's handlers.
    void rename(SPTreeItem* item, const std::string& text);
    void move(SPTreeItem* item, int index);
    void remove(SPTreeItem* item);
    int onDragMove(SPObject* sender, SPEventArgs* args);
    int onDragEnd(SPObject* sender, SPEventArgs* args);
    bool overTree(int x, int y) const;
    void handleDragMove(int x, int y);
    void handleDragEnd(int x, int y);
    void onDragHoverDwell();

    SPTreeViewElement* getTreeView() const { return m_tree; }

private:
    struct Handlers {
        navigation::SidebarNode::RenameHandler onRename;
        navigation::SidebarNode::MoveHandler onMove;
        navigation::SidebarNode::RemoveHandler onRemove;
    };

    void clearDropTarget();
    SPTreeItem* createItem(const navigation::SidebarNode& node);
    const Handlers* handlersFor(SPTreeItem* item) const;

    SPTreeViewElement* m_tree;
    std::string m_selectedUri;
    // Handlers of the nodes shown by the current items.
    std::map<SPTreeItem*, Handlers> m_handlers;
    SPTreeItem* m_dropTarget = nullptr;
};

} // namespace ui
} // namespace space
