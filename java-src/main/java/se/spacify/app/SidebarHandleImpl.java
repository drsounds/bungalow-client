package se.spacify.app;

import se.spacify.navigation.SidebarNode;
import se.spacify.ui.LeftLibraryMenu;

import javax.swing.tree.DefaultMutableTreeNode;
import java.util.List;

/**
 * {@link SidebarHandle} backed by a live {@link DefaultMutableTreeNode} in the
 * {@link LeftLibraryMenu}. Applications only see the interface; this keeps the Swing tree
 * details on the host side.
 */
final class SidebarHandleImpl implements SidebarHandle {

    private final LeftLibraryMenu leftLibraryMenu;
    private final DefaultMutableTreeNode node;

    SidebarHandleImpl(LeftLibraryMenu leftLibraryMenu, DefaultMutableTreeNode node) {
        this.leftLibraryMenu = leftLibraryMenu;
        this.node = node;
    }

    /** The top-level tree node, used by the manager for teardown. */
    DefaultMutableTreeNode node() { return node; }

    @Override
    public SidebarHandle child(String uri) {
        DefaultMutableTreeNode found = find(node, uri);
        return found != null ? new SidebarHandleImpl(leftLibraryMenu, found) : null;
    }

    @Override
    public void setChildren(List<SidebarNode> children) {
        leftLibraryMenu.setNodeChildren(node, children);
    }

    @Override
    public void expand() {
        leftLibraryMenu.expandNode(node);
    }

    private static DefaultMutableTreeNode find(DefaultMutableTreeNode n, String uri) {
        for (int i = 0; i < n.getChildCount(); i++) {
            DefaultMutableTreeNode c = (DefaultMutableTreeNode) n.getChildAt(i);
            if (c.getUserObject() instanceof SidebarNode sn && uri.equals(sn.getUri())) return c;
            DefaultMutableTreeNode deep = find(c, uri);
            if (deep != null) return deep;
        }
        return null;
    }
}
