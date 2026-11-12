#pragma once

#include "SPDragDrop.h"
#include "navigation/SidebarNode.h"

#include <chrono>
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
///
/// It is the drop target of the tree: entities dragged onto one of the user's
/// playlists are added to it, and resting on the playlist for a moment opens
/// it ("itemdraghovered"), so the drop can go to a place in its list instead.
class LeftLibraryMenu : public SPDropTarget {
public:
    explicit LeftLibraryMenu(SPTreeViewElement* tree);
    ~LeftLibraryMenu() override;

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

    // SPDropTarget
    bool dragOver(int x, int y, const SPDragData& data) override;
    void dragLeave() override;
    bool drop(int x, int y, const SPDragData& data) override;

    SPTreeViewElement* getTreeView() const { return m_tree; }

private:
    struct Handlers {
        navigation::SidebarNode::RenameHandler onRename;
        navigation::SidebarNode::MoveHandler onMove;
        navigation::SidebarNode::RemoveHandler onRemove;
    };

    /// The playlist item at window y that a drag can be dropped on, or nullptr.
    SPTreeItem* playlistItemAt(int y) const;
    void setDropTarget(SPTreeItem* item);
    SPTreeItem* createItem(const navigation::SidebarNode& node);
    const Handlers* handlersFor(SPTreeItem* item) const;

    SPTreeViewElement* m_tree;
    std::string m_selectedUri;
    // Handlers of the nodes shown by the current items.
    std::map<SPTreeItem*, Handlers> m_handlers;
    // The playlist item a drag is over, since when, and whether resting on
    // it has opened it already.
    SPTreeItem* m_dropTarget = nullptr;
    std::chrono::steady_clock::time_point m_hoverStarted;
    bool m_hoverOpened = false;
    static constexpr std::chrono::milliseconds kHoverOpenDelay{700};
};

} // namespace ui
} // namespace space
