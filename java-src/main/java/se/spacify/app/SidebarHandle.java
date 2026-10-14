package se.spacify.app;

import se.spacify.navigation.SidebarNode;

import java.util.List;

/**
 * Live handle to a sidebar node a plugin contributed, used to maintain dynamic
 * subtrees (e.g. the Library's Releases/Artists or the Sites bookmarks) without
 * exposing Swing tree internals to plugin code. Removal is handled automatically
 * when the plugin is deactivated.
 */
public interface SidebarHandle {

    /** Handle to a descendant node by its {@code spacify:} uri, or null if absent. */
    SidebarHandle child(String uri);

    /** Replace this node's children and refresh the tree. */
    void setChildren(List<SidebarNode> children);

    /** Expand this node in the tree. */
    void expand();
}
