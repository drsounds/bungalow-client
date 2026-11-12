package se.bungalow.ui;

import se.bungalow.controls.MenuToolButton;
import se.bungalow.controls.ToolBar;
import se.bungalow.controls.ToolMenuItem;
import se.bungalow.library.LibraryEvents;
import se.bungalow.navigation.NavigationListener;
import se.bungalow.navigation.ViewStack;
import se.bungalow.app.library.views.LibraryScanAction;
import se.bungalow.app.media.service.MediaService;
import se.bungalow.app.media.service.MediaServicePlayerComponent;
import se.bungalow.app.music.controls.MusicTable;
import se.bungalow.app.music.model.Playable;
import se.bungalow.app.music.model.PlayableRef;
import se.bungalow.app.playlist.service.PlaylistService;
import se.bungalow.navigation.SidebarNode;
import se.bungalow.service.media.PlaybackCoordinator;
import se.bungalow.ui.theme.ThemeManager;
import se.bungalow.controls.Panel;
import javax.swing.*;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeCellRenderer;
import javax.swing.tree.DefaultTreeModel;
import javax.swing.tree.TreePath;

import se.bungalow.controls.Tree;
import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.util.List;

public class LeftLibraryMenu extends Panel implements NavigationListener {

    private final Tree tree;
    private final JScrollPane scroll;
    private final DefaultMutableTreeNode root;
    private boolean suppressSelection = false;
	private ToolBar toolbar;
	private JTextField searchField;
	private ToolBar bottomToolbar;
	private MenuToolButton addToLibraryMenuButton;
	@SuppressWarnings("unused")
	private JPopupMenu addToLibraryMenu;
    private ViewStack viewStack;
	private final JPanel playerHost = new JPanel(new BorderLayout());
	private MediaServicePlayerComponent currentPlayer;

	public ViewStack getViewStack() {
        return viewStack;
    }

    @Override
    protected void paintSurface(Graphics g) {
        Graphics2D g2 = (Graphics2D) g.create();
        getSkin().paintLeftLibraryMenu(this, g2);
        g2.dispose();
        super.paintSurface(g);
    }

    public void reload() {
	}

	public void onAdd() {
	}

    private void setActivePlayer(MediaService Service) {
        MediaServicePlayerComponent next = Service != null ? Service.getPlayerComponent() : null;
        if (next == currentPlayer) return;

        if (currentPlayer != null) {
            currentPlayer.onDeactivated();
            playerHost.remove(currentPlayer.getComponent());
        }
        currentPlayer = next;
        if (currentPlayer != null) {
            playerHost.add(currentPlayer.getComponent(), BorderLayout.CENTER);
            currentPlayer.onActivated();
        }
        playerHost.setVisible(currentPlayer != null);
        playerHost.revalidate();
        playerHost.repaint();
    }

    public LeftLibraryMenu(ViewStack viewStack) {
        this.viewStack = viewStack;
        getComponent().setLayout(new BorderLayout());
        getComponent().setPreferredSize(new Dimension(220, 0));
        getComponent().setOpaque(true);

        // Core shell nodes only; content subtrees (Your Library, Sites) are
        // contributed by built-in apps via addSidebarNode() at activation.
        root = new DefaultMutableTreeNode("root");
        root.add(nodeFor(new SidebarNode("Player",    "bungalow:now-playing")));
        root.add(nodeFor(new SidebarNode("Downloads", "bungalow:downloads")));
        root.add(nodeFor(new SidebarNode("Apps",   "bungalow:apps")));

        toolbar = new ToolBar();
        toolbar.getComponent().setFloatable(false);
        toolbar.getComponent().setOpaque(true);
        toolbar.getComponent().setBackground(ThemeManager.getTintColor());
        add(toolbar, BorderLayout.NORTH);

        searchField = new JTextField();
        searchField.putClientProperty("JTextField.placeholderText", "Search...");
        searchField.setPreferredSize(new Dimension(180, 28));
        searchField.addActionListener(e -> {
            String q = searchField.getText().trim();
            if (!q.isEmpty()) {
                String encoded = URLEncoder.encode(q, StandardCharsets.UTF_8);
                viewStack.navigate("bungalow:search?q=" + encoded);
            }
        });
        toolbar.getComponent().add(searchField);

        tree = new Tree(root);
        tree.getComponent().setOpaque(false);
        tree.getComponent().setRootVisible(false);
        tree.getComponent().setShowsRootHandles(true);
        tree.getComponent().setBorder(BorderFactory.createEmptyBorder(8, 4, 8, 4));
        tree.getComponent().setRowHeight(28);
        tree.getComponent().setCellRenderer(new ThemedTreeCellRenderer());

        tree.getComponent().addMouseListener(new MouseAdapter() {
            @Override
            public void mouseClicked(MouseEvent e) {
                JTree jt = tree.getComponent();
                // Resolve the row from the y-position only, so a click anywhere along
                // the row width counts — not just on the label text.
                int row = jt.getClosestRowForLocation(e.getX(), e.getY());
                if (row < 0) return;
                Rectangle bounds = jt.getRowBounds(row);
                if (bounds == null || e.getY() < bounds.y || e.getY() >= bounds.y + bounds.height) {
                    return;   // clicked in the empty area below the last row
                }
                jt.setSelectionRow(row);
                TreePath path = jt.getPathForRow(row);
                DefaultMutableTreeNode node = (DefaultMutableTreeNode) path.getLastPathComponent();
                if (node.getUserObject() instanceof SidebarNode sn && sn.getUri() != null) {
                    suppressSelection = true;
                    viewStack.navigate(sn.getUri());
                    suppressSelection = false;
                } else if (!node.isLeaf()) {
                    // A folder row (no URI): toggle it across the whole width.
                    if (jt.isExpanded(row)) jt.collapseRow(row); else jt.expandRow(row);
                }
            }
        });

        // Accept content dragged out of any music list, dropped onto a playlist node.
        tree.getComponent().setDropMode(DropMode.ON);
        tree.getComponent().setTransferHandler(new PlaylistDropHandler());

        scroll = new JScrollPane(tree.getComponent());
        scroll.setBorder(BorderFactory.createEmptyBorder());
        // Keep scroll and viewport opaque so they paint a background;
        // colours are kept in sync by updateColors() below.
        scroll.setOpaque(true);
        scroll.getViewport().setOpaque(true);
        getComponent().add(scroll, BorderLayout.CENTER);

        bottomToolbar = new ToolBar();
        // Cross-app download activity spinner (listens on the broadcast bus).
        bottomToolbar.getComponent().add(new DownloadActivityIndicator());
        

        // The active media Service's player surface sits below the queue and above
        // the toolbar; it's part of this sticky panel, so navigation never hides it.
        playerHost.setOpaque(false);
        playerHost.setVisible(true);

        JPanel south = new JPanel();
        south.setOpaque(false);
        south.setLayout(new BoxLayout(south, BoxLayout.PAGE_AXIS));
        south.add(playerHost);
        south.add(bottomToolbar.getComponent());
        getComponent().add(south, BorderLayout.SOUTH);
 

        addToLibraryMenuButton = new MenuToolButton();
        addToLibraryMenuButton.getComponent().setText("Add to Library");
        addToLibraryMenu = addToLibraryMenuButton.getPopup();
        ToolMenuItem addBtn    = new ToolMenuItem("Add");
        ToolMenuItem scanBtn   = new ToolMenuItem("Scan…");

        scanBtn.getComponent().addActionListener(e -> LibraryScanAction.run(getComponent(),
            () -> { /*reload();*/ LibraryEvents.fireChanged(); }));
        addBtn.getComponent().addActionListener(e -> { onAdd(); reload(); LibraryEvents.fireChanged(); });
        addToLibraryMenuButton.add(addBtn);
        addToLibraryMenuButton.add(scanBtn);

        bottomToolbar.add(addToLibraryMenuButton);

        viewStack.addNavigationListener(this);

        setActivePlayer(PlaybackCoordinator.getActiveService());
        PlaybackCoordinator.addActiveServiceListener(ms ->
            SwingUtilities.invokeLater(() -> setActivePlayer(ms)));

        updateColors();
        ThemeManager.addChangeListener(this::updateColors);
    }

    // ── Theme-aware colour sync ───────────────────────────────────────────────

    private void updateColors() {
        Color bg = ThemeManager.getBackground();
        getComponent().setBackground(bg);
        tree.getComponent().setBackground(bg);
        scroll.setBackground(bg);
        scroll.getViewport().setBackground(bg);
        tree.repaint();
    }

    // ── Custom cell renderer ──────────────────────────────────────────────────

    private static final class ThemedTreeCellRenderer extends DefaultTreeCellRenderer {
        private static final long serialVersionUID = -3474131119330495091L;

		@Override
        public Component getTreeCellRendererComponent(JTree tree, Object value,
                boolean sel, boolean expanded, boolean leaf, int row, boolean hasFocus) {
            super.getTreeCellRendererComponent(tree, value, sel, expanded, leaf, row, hasFocus);
            // Transparent: the full-width selection band is painted by the Tree, so the
            // renderer must not paint its own label-width background box or focus border.
            setOpaque(false);
            setBackgroundSelectionColor(null);
            setBackgroundNonSelectionColor(null);
            setBorderSelectionColor(null);
            setForeground(sel ? Color.WHITE : ThemeManager.getForeground());
            // Per-node favicon (bookmarks); falls back to the default tree icon.
            if (value instanceof DefaultMutableTreeNode n
                    && n.getUserObject() instanceof SidebarNode sn && sn.getIcon() != null) {
                setIcon(sn.getIcon());
            }
            return this;
        }
    }

    // ── LeftLibraryMenu tree model ────────────────────────────────────────────────────

    private DefaultMutableTreeNode nodeFor(SidebarNode sn) {
        DefaultMutableTreeNode node = new DefaultMutableTreeNode(sn);
        for (SidebarNode child : sn.getChildren()) node.add(nodeFor(child));
        return node;
    }

    public DefaultMutableTreeNode getRootNode() { return root; }

    /** Append an app-contributed node (subtree) to the sidebar root, live. */
    public DefaultMutableTreeNode addSidebarNode(SidebarNode sn) {
        DefaultMutableTreeNode node = nodeFor(sn);
        root.add(node);
        ((DefaultTreeModel) tree.getComponent().getModel()).nodeStructureChanged(root);
        return node;
    }

    /** Remove a node previously added via {@link #addSidebarNode}. */
    public void removeSidebarNode(DefaultMutableTreeNode node) {
        if (node.getParent() != null) {
            root.remove(node);
            ((DefaultTreeModel) tree.getComponent().getModel()).nodeStructureChanged(root);
        }
    }

    /** Replace a node's children (used for dynamic app subtrees). */
    public void setNodeChildren(DefaultMutableTreeNode node, List<SidebarNode> children) {
        node.removeAllChildren();
        for (SidebarNode sn : children) node.add(nodeFor(sn));
        ((DefaultTreeModel) tree.getComponent().getModel()).nodeStructureChanged(node);
    }

    /** Expand a node in the tree. */
    public void expandNode(DefaultMutableTreeNode node) {
        tree.getComponent().expandPath(new TreePath(node.getPath()));
    }

    // ── NavigationListener ────────────────────────────────────────────────────

    @Override
    public void onNavigate(String uri, boolean canGoBack, boolean canGoForward) {
        if (suppressSelection) return;
        selectNodeForUri(uri, (DefaultMutableTreeNode) tree.getComponent().getModel().getRoot());
    }

    private boolean selectNodeForUri(String uri, DefaultMutableTreeNode node) {
        if (node.getUserObject() instanceof SidebarNode sn && uri.equals(sn.getUri())) {
            TreePath path = new TreePath(node.getPath());
            tree.getComponent().setSelectionPath(path);
            tree.getComponent().scrollPathToVisible(path);
            return true;
        }
        for (int i = 0; i < node.getChildCount(); i++) {
            if (selectNodeForUri(uri, (DefaultMutableTreeNode) node.getChildAt(i))) return true;
        }
        return false;
    }

    // ── Drop-to-playlist ──────────────────────────────────────────────────────

    private static final String PLAYLIST_PREFIX = "bungalow:playlist:";

    /** Accepts a {@link MusicTable.PlaylistDrag} dropped onto a playlist node. */
    private final class PlaylistDropHandler extends TransferHandler {
        private static final long serialVersionUID = 1L;

        @Override
        public boolean canImport(TransferSupport support) {
            return support.isDrop()
                && support.isDataFlavorSupported(MusicTable.PLAYABLE_REF_FLAVOR)
                && playlistIdAt(support) != null;
        }

        @Override
        public boolean importData(TransferSupport support) {
            if (!canImport(support)) return false;
            String id = playlistIdAt(support);
            PlaylistService svc = editablePlaylistOwning(id);
            if (svc == null) return false;
            try {
                MusicTable.PlaylistDrag drag = (MusicTable.PlaylistDrag)
                    support.getTransferable().getTransferData(MusicTable.PLAYABLE_REF_FLAVOR);
                if (drag == null || drag.ref() == null) return false;
                PlayableRef ref = drag.ref();
                if (drag.expand() && ref.isExpandable()) {
                    for (Playable child : ref.expansion()) svc.addToPlaylist(id, child);
                } else {
                    svc.addToPlaylist(id, ref, ref.getKind() != null ? ref.getKind().id() : null);
                }
                return true;
            } catch (Exception e) {
                return false;
            }
        }
    }

    /** The playlist id of the node under the drop, or null if it isn't a playlist node. */
    private String playlistIdAt(TransferHandler.TransferSupport support) {
        if (!(support.getDropLocation() instanceof JTree.DropLocation dl)) return null;
        TreePath path = dl.getPath();
        if (path == null) return null;
        if (path.getLastPathComponent() instanceof DefaultMutableTreeNode n
                && n.getUserObject() instanceof SidebarNode sn) {
            String uri = sn.getUri();
            if (uri != null && uri.startsWith(PLAYLIST_PREFIX) && !uri.equals(PLAYLIST_PREFIX + "new"))
                return uri.substring(PLAYLIST_PREFIX.length());
        }
        return null;
    }

    /** The editable playlist service that owns {@code id}, or null. */
    private PlaylistService editablePlaylistOwning(String id) {
        for (PlaylistService svc : viewStack.getMainWindow().getServiceManager().getServices(PlaylistService.class)) {
            try {
                if (svc.isEditable() && svc.getPlaylist(id) != null) return svc;
            } catch (Exception ignored) {}
        }
        return null;
    }
}
