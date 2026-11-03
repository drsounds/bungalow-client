#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace space {
namespace navigation {

/// One entry of the sidebar, independent of the tree widget that shows it.
///
/// A top-level node without a URI is a section ("MAIN", "SITES", …): its
/// children are listed under the section header. Sections with the same label
/// contributed by different features are merged.
class SidebarNode {
public:
    using RenameHandler = std::function<void(const std::string&)>;
    using MoveHandler = std::function<void(int)>;
    using RemoveHandler = std::function<void()>;

    explicit SidebarNode(std::string label, std::string uri = {})
        : m_label(std::move(label)), m_uri(std::move(uri)) {}

    const std::string& getLabel() const { return m_label; }
    const std::string& getUri() const { return m_uri; }
    bool isSection() const { return m_uri.empty(); }

    SidebarNode& addChild(SidebarNode child) {
        m_children.push_back(std::move(child));
        return *this;
    }
    const std::vector<SidebarNode>& getChildren() const { return m_children; }
    bool isLeaf() const { return m_children.empty(); }

    const std::string& getIconPath() const { return m_iconPath; }
    SidebarNode& setIconPath(std::string path) {
        m_iconPath = std::move(path);
        return *this;
    }

    /// Shown with its children visible.
    bool isExpanded() const { return m_expanded; }
    SidebarNode& setExpanded(bool expanded) {
        m_expanded = expanded;
        return *this;
    }

    /// Created from a user bookmark, so it can be removed from the sidebar.
    bool isBookmark() const { return m_bookmark; }
    SidebarNode& setBookmark(bool bookmark) {
        m_bookmark = bookmark;
        return *this;
    }

    /// A section listed after a blank gap instead of under its label.
    bool isSpacer() const { return m_spacer; }
    SidebarNode& setSpacer(bool spacer) {
        m_spacer = spacer;
        return *this;
    }

    /// Clicking shows an inline input instead of navigating; the entered
    /// text goes to the rename handler.
    bool isEditOnClick() const { return m_editOnClick; }
    SidebarNode& setEditOnClick(bool editOnClick) {
        m_editOnClick = editOnClick;
        return *this;
    }

    /// Can be renamed in place (from its context menu, or by clicking it if
    /// edit-on-click); the handler gets the entered text.
    const RenameHandler& getOnRename() const { return m_onRename; }
    SidebarNode& setOnRename(RenameHandler handler) {
        m_onRename = std::move(handler);
        return *this;
    }

    /// Can be dragged among the adjacent movable nodes of its section; the
    /// handler gets the node's new index among them.
    const MoveHandler& getOnMove() const { return m_onMove; }
    SidebarNode& setOnMove(MoveHandler handler) {
        m_onMove = std::move(handler);
        return *this;
    }

    /// Can be deleted from its context menu.
    const RemoveHandler& getOnRemove() const { return m_onRemove; }
    SidebarNode& setOnRemove(RemoveHandler handler) {
        m_onRemove = std::move(handler);
        return *this;
    }

private:
    std::string m_label;
    std::string m_uri;
    std::string m_iconPath;
    std::vector<SidebarNode> m_children;
    bool m_expanded = false;
    bool m_bookmark = false;
    bool m_spacer = false;
    bool m_editOnClick = false;
    RenameHandler m_onRename;
    MoveHandler m_onMove;
    RemoveHandler m_onRemove;
};

} // namespace navigation
} // namespace space
