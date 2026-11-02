#pragma once

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

private:
    std::string m_label;
    std::string m_uri;
    std::string m_iconPath;
    std::vector<SidebarNode> m_children;
    bool m_expanded = false;
    bool m_bookmark = false;
};

} // namespace navigation
} // namespace space
