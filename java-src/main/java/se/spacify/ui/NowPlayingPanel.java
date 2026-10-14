package se.spacify.ui;

import se.spacify.controls.Table;
import se.spacify.controls.Panel;
import se.spacify.controls.ToolBar;
import se.spacify.controls.ScrollPane;
import se.spacify.controls.Label;
import se.spacify.navigation.ViewStack;
import se.spacify.service.media.PlaybackCoordinator;
import se.spacify.service.media.PlayQueue;
import se.spacify.service.media.PlayQueueItem;
import se.spacify.service.media.PlayRequest;
import se.spacify.ui.theme.ThemeManager;

import javax.swing.*;
import javax.swing.border.EmptyBorder;
import javax.swing.table.DefaultTableCellRenderer;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.List;

/**
 * Right-hand play-queue panel. Mirrors the library table styling (theme colours,
 * striped rows, accent selection) but lists the active {@link PlayQueue} with
 * Name / Artists / Duration columns. The currently-playing entry is highlighted,
 * and double-clicking a row jumps playback to that position.
 */
public class NowPlayingPanel extends Panel {

	private final DefaultTableModel model;
    private final Table table;
    private final ScrollPane       scroll;
    private final Label            emptyLabel;
	private ToolBar topToolbar;
	private ToolBar bottomToolbar;

    public NowPlayingPanel(ViewStack viewStack) {
        getComponent().setLayout(new BorderLayout(0, 0));
        getComponent().setPreferredSize(new Dimension(220, 0));
        getComponent().setOpaque(false);   // we paint our own gradient in paintSurface

        topToolbar = new ToolBar();
        topToolbar.getComponent().setFloatable(false);
        topToolbar.getComponent().setOpaque(true);
        topToolbar.getComponent().setBackground(ThemeManager.getTintColor());
        add(topToolbar);

        JButton title = new JButton(UIManager.getIcon("FileView.fileIcon"));
        title.setFont(title.getFont().deriveFont(Font.BOLD, 11f));
        
        // ── Queue table ──────────────────────────────────────────────────────
        model = new DefaultTableModel(new String[]{"Name", "Duration"}, 0) {
            @Override public boolean isCellEditable(int r, int c) { return false; }
        };
        table = new Table(model);
        table.getComponent().setFillsViewportHeight(true);
        table.getComponent().setShowGrid(false);
        table.getComponent().setIntercellSpacing(new Dimension(0, 0));
        table.getComponent().setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        table.getComponent().getColumnModel().getColumn(1).setMaxWidth(64);
        // Transparent so the panel's gradient shows through behind the rows.
        table.getComponent().setOpaque(false);

        QueueCellRenderer renderer = new QueueCellRenderer();
        for (int i = 0; i < table.getComponent().getColumnCount(); i++) {
            table.getComponent().getColumnModel().getColumn(i).setCellRenderer(renderer);
        }
        table.getComponent().addMouseListener(new MouseAdapter() {
            @Override public void mouseClicked(MouseEvent e) {
                if (e.getClickCount() == 2) {
                    int row = table.getComponent().rowAtPoint(e.getPoint());
                    if (row >= 0) PlayQueue.getInstance().playAt(row);
                }
            }

            @Override public void mousePressed(MouseEvent e)  { maybeShowQueueMenu(e); }
            @Override public void mouseReleased(MouseEvent e) { maybeShowQueueMenu(e); }
        });

        scroll = new ScrollPane(table);
        // Non-UIResource empty border so the Nimbus reinstall on theme change
        // doesn't re-install a default scroll-pane border.
        scroll.getComponent().setBorder(BorderFactory.createEmptyBorder());
        // Transparent viewport/scroll-pane so the gradient shows behind the rows.
        scroll.getComponent().setOpaque(false);
        scroll.getComponent().getViewport().setOpaque(false);
        // Repaint the whole viewport on scroll so the gradient doesn't smear.
        scroll.getComponent().getViewport().setScrollMode(JViewport.SIMPLE_SCROLL_MODE);

        emptyLabel = new Label("Nothing playing");
        emptyLabel.getComponent().setForeground(Color.WHITE);
        emptyLabel.getComponent().setFont(emptyLabel.getComponent().getFont().deriveFont(12f));
        emptyLabel.getComponent().setHorizontalAlignment(SwingConstants.CENTER);

        add(topToolbar,  BorderLayout.NORTH);
        add(scroll, BorderLayout.CENTER);

        bottomToolbar = new ToolBar();
        bottomToolbar.getComponent().setFloatable(false);
        bottomToolbar.getComponent().setOpaque(true);
        bottomToolbar.getComponent().setBackground(ThemeManager.getTintColor());
        bottomToolbar.getComponent().add(new JButton("Sync"));

        Panel south = new Panel();
        south.getComponent().setOpaque(false);
        south.getComponent().setLayout(new BoxLayout(south.getComponent(), BoxLayout.PAGE_AXIS));
        south.add(bottomToolbar);
        add(south, BorderLayout.SOUTH);

        topToolbar.getComponent().add(title);

        updateColors();
        refresh();

        PlayQueue.getInstance().addChangeListener(() ->
            SwingUtilities.invokeLater(this::refresh));
        ThemeManager.addChangeListener(() -> { updateColors(); refresh(); });
    }

    /** Right-click a queue entry to re-pick which Service plays it ("Play with…"). */
    private void maybeShowQueueMenu(MouseEvent e) {
        if (!e.isPopupTrigger()) return;
        int row = table.getComponent().rowAtPoint(e.getPoint());
        if (row < 0) return;
        table.getComponent().setRowSelectionInterval(row, row);
        List<PlayQueueItem> items = PlayQueue.getInstance().getItems();
        if (row >= items.size()) return;
        PlayRequest req = items.get(row).getSource();
        if (req == null) return;
        JPopupMenu menu = new JPopupMenu();
        JMenuItem playWith = new JMenuItem("Play with…");
        playWith.addActionListener(a -> PlaybackCoordinator.resolveAndPlay(req, true));
        menu.add(playWith);
        menu.show(e.getComponent(), e.getX(), e.getY());
    }

    /** Rebuild the table rows from the current queue and keep the selection on the playing row. */
    private void refresh() {
        List<PlayQueueItem> items = PlayQueue.getInstance().getItems();
        model.setRowCount(0);
        for (PlayQueueItem it : items) {
            model.addRow(new Object[]{
                it.getName(), fmtDuration(it.getDurationMs())
            });
        }
        int current = PlayQueue.getInstance().getCurrentIndex();
        if (current >= 0 && current < model.getRowCount()) {
            table.getComponent().setRowSelectionInterval(current, current);
        } else {
            table.getComponent().clearSelection();
        }

        boolean empty = items.isEmpty();
        // Show the placeholder in the centre while the queue is empty.
        if (empty && scroll.getParent() == this) {
            remove(scroll);
            add(emptyLabel, BorderLayout.CENTER);
        } else if (!empty && emptyLabel.getParent() == this) {
            remove(emptyLabel);
            add(scroll, BorderLayout.CENTER);
        }
        revalidate();
        repaint();
    }

    private void updateColors() {
        // This sticky panel always renders white text on its accent gradient,
        // regardless of the light/dark theme setting.
        Color grid = ThemeManager.getGridColor();
        table.getComponent().setForeground(Color.WHITE);
        table.getComponent().setGridColor(grid);
        table.repaint();
    }

    @Override
    protected void paintSurface(Graphics g) {
        Graphics2D g2 = (Graphics2D) g.create();
        getSkin().paintPlaylist(this, g2);
        g2.dispose();
    }

    private static String fmtDuration(long ms) {
        if (ms <= 0) return "";
        long s = ms / 1000;
        return String.format("%d:%02d", s / 60, s % 60);
    }

    /**
     * Renderer that keeps the gradient visible and never stripes rows. The playing
     * row keeps its distinct light-green-on-black highlight; every other row is
     * transparent with white text, regardless of the light/dark theme.
     */
    private static final class QueueCellRenderer extends DefaultTableCellRenderer {

        private static final long serialVersionUID = 5721808300261876002L;

		@Override
        public Component getTableCellRendererComponent(JTable table, Object value,
                boolean isSelected, boolean hasFocus, int row, int column) {
            super.getTableCellRendererComponent(table, value, isSelected, hasFocus, row, column);
            boolean playing = row == PlayQueue.getInstance().getCurrentIndex();
            if (playing) {
                setOpaque(true);
                setBackground(ThemeManager.getNowPlayingBackground());
                setForeground(ThemeManager.getNowPlayingForeground());
            } else {
                setOpaque(false);   // let the gradient show through
                setForeground(Color.WHITE);
            }
            setBorder(BorderFactory.createEmptyBorder(0, 6, 0, 6));
            return this;
        }
    }
}
