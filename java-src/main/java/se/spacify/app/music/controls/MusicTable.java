package se.spacify.app.music.controls;

import se.spacify.controls.GroupedListPanel;
import se.spacify.controls.Table;
import se.spacify.graphics.GroupAvatar;
import se.spacify.navigation.ViewStack;
import se.spacify.service.media.AvailabilityResolver;
import se.spacify.service.media.PlaybackCoordinator;
import se.spacify.service.media.PlayQueue;
import se.spacify.service.media.PlayQueueItem;
import se.spacify.service.media.PlayRequest;
import se.spacify.service.media.TrackAvailability;
import se.spacify.app.music.model.PlayableKind;
import se.spacify.app.music.model.PlayableRef;
import se.spacify.ui.theme.ThemeManager;
import se.spacify.ui.theme.ThemedTableCellRenderer;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableColumn;
import java.awt.*;
import java.awt.datatransfer.DataFlavor;
import java.awt.datatransfer.Transferable;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * A reusable, theme-aware music list table that any Spacify app can embed without
 * subclassing a view. It presents a {@link Table} whose rows are playable items
 * (tracks, downloads, catalogue results) and adds the music-specific chrome that
 * should look and behave the same everywhere:
 * <ul>
 *   <li>a trailing Buy/Stream split-button column driven by per-row
 *       {@link TrackAvailability} (or just a file glyph when matched locally);</li>
 *   <li>an optional library-membership (✓/＋) column;</li>
 *   <li>double-click to turn the visible rows into the play queue;</li>
 *   <li>a right-click "Play with…" menu; and</li>
 *   <li>an optional grouped (album/playlist) presentation.</li>
 * </ul>
 * The embedding code fills the {@linkplain #getModel() model} and supplies per-row
 * playback metadata and callbacks through a {@link Source}. Call {@link #refresh()}
 * after (re)populating the model.
 */
public class MusicTable extends JPanel {

	private static final long serialVersionUID = 1L;

	/** Width of the Buy/Stream column, and the ▾ (menu) hit zone within it. */
	private static final int BUY_STREAM_COL_WIDTH = 94;
	private static final int BUY_STREAM_ARROW_W   = 18;
	private static final int LIB_TOGGLE_COL_WIDTH = 32;

	/** Column key of the managed library-membership (✓/＋) toggle column. */
	public static final String LIBRARY_TOGGLE_COLUMN = "spacify:library-toggle";
	/** Column key of the managed trailing Buy/Stream column. */
	public static final String BUY_STREAM_COLUMN     = "spacify:buy-stream";

	/**
	 * A caller-defined column: a stable {@code key} (used to address the column and
	 * to bind row values, so a later view can reorder or drop columns without
	 * touching its row-building code) and the {@code title} shown in the header.
	 */
	public record Column(String key, String title) {}

	/**
	 * Builder for a keyed table row. Values are bound to columns by key — order is
	 * irrelevant and unset columns are left blank — so the same row-building code
	 * works across views that present the columns in different orders or subsets.
	 * Unlike {@link Map#of}, {@code null} values are allowed.
	 */
	public static final class Row {
		private final Map<String, Object> values = new HashMap<>();
		/** Bind {@code value} to the column with the given key; returns {@code this}. */
		public Row set(String key, Object value) {
			values.put(key, value);
			return this;
		}
	}

	/** Start a new keyed {@link Row}; fill it with {@link Row#set} and pass to {@link #addRow(Row)}. */
	public static Row row() {
		return new Row();
	}

	/**
	 * Per-row playback metadata and interaction callbacks supplied by the embedding
	 * app. Only {@link #playRequestAt(int)} is required; the rest have sensible
	 * defaults, so a read-only list need implement just that one method.
	 */
	public interface Source {

		/** A {@link PlayRequest} for the model row, or {@code null} if it isn't playable. */
		PlayRequest playRequestAt(int row);

		/**
		 * A {@link PlayQueueItem} for the model row, or {@code null}. By default this
		 * derives from {@link #playRequestAt} (routing playback through the
		 * "Play with…" resolver); views that play a concrete target may override it.
		 */
		default PlayQueueItem queueItemAt(int row) {
			PlayRequest req = playRequestAt(row);
			if (req == null)
				return null;
			return new PlayQueueItem(req.key(), req.title(), req.artist(), req.durationMs(),
					() -> PlaybackCoordinator.resolveAndPlay(req), req);
		}

		/**
		 * A draggable, kind-tagged reference for the model row — the payload when the
		 * row is dragged into a playlist — or {@code null} if the row can't be added.
		 * The default derives a {@link PlayableKind#TRACK} reference from
		 * {@link #playRequestAt} (kind inferred from its play URI), with no expansion;
		 * views whose rows are releases/playlists override this to tag the kind and
		 * supply the expansion added when the drop's expand modifier is held.
		 */
		default PlayableRef dragItemAt(int row) {
			PlayRequest req = playRequestAt(row);
			if (req == null)
				return null;
			String uri = req.key();
			return new PlayableRef(PlayableKind.fromUri(uri), uri,
					req.title(), req.artist(), req.durationMs(), List.of());
		}

		/** Whether the row's track is saved in the local library (drives ✓ vs ＋). */
		default boolean inLibraryAt(int row) { return false; }

		/** Add/remove the row's track to/from the local library. */
		default void toggleLibraryAt(int row) {}

		/** Fallback when the view produces no play-queue items (e.g. open a detail). */
		default void onActivate(int row) {}

		/**
		 * Single-click on a cell that isn't one of the managed columns. {@code columnKey}
		 * is the {@link Column#key()} of the clicked column (never a positional index),
		 * or {@code null} if it couldn't be resolved.
		 */
		default void onCellClicked(int row, String columnKey) {}

		/** Grouping options for the grouped presentation; read lazily, may be empty. */
		default List<Grouping> groupings() { return List.of(); }
	}

	/**
	 * Identity and display text of the group a row belongs to. {@code key} is the
	 * stable group identity (drives the placeholder image colour and de-dupes rows
	 * into sections); {@code title}/{@code subtitle} are shown in the section header.
	 */
	public record GroupRef(String key, String title, String subtitle) {}

	/** A named way of grouping the current rows (e.g. "Release", "Playlist"). */
	public interface Grouping {
		String name();

		/** The group the given model row belongs to. */
		GroupRef groupOf(int modelRow);
	}

	/** Notified when the user drags a flat-table row from one position to another. */
	public interface ReorderHandler {
		/** Move the item at {@code fromRow} so it lands at {@code toRow} (post-removal index). */
		void reorder(int fromRow, int toRow);
	}

	/** Notified when a {@link PlayableRef} from another list is dropped into this table. */
	public interface AddHandler {
		/**
		 * Add {@code ref} at insert index {@code index} (0..rowCount, the position the
		 * drop indicated); when {@code expand} is set and the ref is expandable, its
		 * {@link PlayableRef#expansion()} children are added instead of the ref itself.
		 */
		void add(PlayableRef ref, boolean expand, int index);
	}

	/**
	 * The cross-view drag payload: a {@link PlayableRef} plus whether the expand
	 * modifier was held at drag start. Exposed (with {@link #PLAYABLE_REF_FLAVOR})
	 * so external drop targets — e.g. the sidebar's playlist nodes — can accept a
	 * row dragged out of any music list.
	 */
	public record PlaylistDrag(PlayableRef ref, boolean expand) {}

	/** Local-JVM data flavor carrying a {@link PlaylistDrag} out of a music list. */
	public static final DataFlavor PLAYABLE_REF_FLAVOR =
		new DataFlavor(PlaylistDrag.class, "application/x-spacify-playlist-drag");

	private final ViewStack viewStack;
	private final Source source;
	private final DefaultTableModel model;
	private final Table table;
	private final javax.swing.JTable jtable;
	private final JScrollPane scroll;
	/** Index of the trailing Buy/Stream column appended to the model. */
	private final int buyStreamCol;
	/** Index of the optional library-membership (✓/＋) column, or -1 if absent. */
	private final int libToggleCol;
	/** Column key → model column index, for addressing columns without positions. */
	private final Map<String, Integer> indexByKey;
	/** Model column index → column key (the inverse of {@link #indexByKey}). */
	private final String[] keyByIndex;

	// ── Optional grouped presentation ───────────────────────────────────────────
	private GroupedListPanel groupedPanel;
	private List<Grouping> cachedGroupings;
	private Grouping currentGrouping;
	private boolean grouped;
	/** Coalesces bursts of model changes into a single managed-column sync. */
	private boolean managedSyncPending;

	// ── Drag-and-drop: reorder within, plus export/import of playlist refs ───────
	/** Notified when a row is dragged to a new position; null disables reordering. */
	private ReorderHandler reorderHandler;
	/** Notified when a ref from another list is dropped here; null disables adding. */
	private AddHandler addHandler;
	/** Whether the expand modifier (Alt) was down at the last mouse press (drag start). */
	private boolean lastPressExpand;
	private static final DataFlavor ROW_INDEX_FLAVOR =
		new DataFlavor(Integer.class, "application/x-spacify-row-index");

	/**
	 * @param viewStack          used to navigate to a store when "Buy" is chosen
	 * @param columns            the app's own columns (key + title); the managed
	 *                           (library-toggle / Buy-Stream) columns are appended.
	 *                           A later view can present these in any order or drop
	 *                           some, and addressing stays stable via the keys.
	 * @param showLibraryToggle  whether to include the ✓/＋ membership column
	 * @param source             per-row playback metadata and callbacks
	 */
	public MusicTable(ViewStack viewStack, List<Column> columns, boolean showLibraryToggle, Source source) {
		super(new BorderLayout());
		this.viewStack = viewStack;
		this.source = source;
		setOpaque(false);

		// A trailing Buy/Stream column is appended to every table; its cells are
		// rendered from per-row availability, not the row data, so rows added with
		// the caller's column count are padded to fit. An optional library-toggle
		// column precedes it. Each column carries a stable key so rows are bound by
		// key (see addRow) and views can reorder/omit columns freely.
		String[] cols = new String[columns.size() + (showLibraryToggle ? 1 : 0) + 1];
		keyByIndex = new String[cols.length];
		indexByKey = new LinkedHashMap<>();
		for (int i = 0; i < columns.size(); i++) {
			Column c = columns.get(i);
			cols[i] = c.title();
			keyByIndex[i] = c.key();
			indexByKey.put(c.key(), i);
		}
		int idx = columns.size();
		if (showLibraryToggle) {
			libToggleCol = idx;
			cols[idx] = "";
			keyByIndex[idx] = LIBRARY_TOGGLE_COLUMN;
			indexByKey.put(LIBRARY_TOGGLE_COLUMN, idx);
			idx++;
		} else {
			libToggleCol = -1;
		}
		buyStreamCol = idx;
		cols[idx] = "";
		keyByIndex[idx] = BUY_STREAM_COLUMN;
		indexByKey.put(BUY_STREAM_COLUMN, idx);
		model = new DefaultTableModel(cols, 0) {
			private static final long serialVersionUID = 1L;
			@Override
			public boolean isCellEditable(int r, int c) {
				return false;
			}
			@Override
			public void addRow(Object[] rowData) {
				if (rowData != null && rowData.length < getColumnCount()) {
					Object[] padded = new Object[getColumnCount()];
					System.arraycopy(rowData, 0, padded, 0, rowData.length);
					rowData = padded;
				}
				super.addRow(rowData);
			}
		};
		table = new Table(model);
		jtable = table.getComponent();
		jtable.setFillsViewportHeight(true);
		jtable.setShowGrid(false);
		jtable.setIntercellSpacing(new Dimension(0, 0));
		jtable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
		jtable.addMouseListener(new MouseAdapter() {
			@Override
			public void mouseClicked(MouseEvent e) {
				int row = jtable.rowAtPoint(e.getPoint());
				if (row < 0)
					return;
				int viewCol = jtable.columnAtPoint(e.getPoint());
				int col = viewCol < 0 ? -1 : jtable.convertColumnIndexToModel(viewCol);
				if (e.getClickCount() == 2) {
					activate(row);
				} else if (e.getClickCount() == 1) {
					if (col == libToggleCol && handleLibraryToggleClick(row)) return;
					if (col == buyStreamCol && handleBuyStreamClick(row, e)) return;
					source.onCellClicked(row, keyForColumn(col));
				}
			}

			@Override public void mousePressed(MouseEvent e)  { lastPressExpand = e.isAltDown(); maybeShowRowMenu(e); }
			@Override public void mouseReleased(MouseEvent e) { maybeShowRowMenu(e); }
		});

		ThemedTableCellRenderer renderer = new ThemedTableCellRenderer();
		for (int i = 0; i < jtable.getColumnCount(); i++) {
			jtable.getColumnModel().getColumn(i).setCellRenderer(renderer);
		}
		// The Buy/Stream column draws its own split button.
		TableColumn bs = jtable.getColumnModel().getColumn(buyStreamCol);
		bs.setCellRenderer(new BuyStreamRenderer());
		bs.setResizable(false);
		bs.setMinWidth(0);
		bs.setMaxWidth(BUY_STREAM_COL_WIDTH);
		bs.setPreferredWidth(BUY_STREAM_COL_WIDTH);
		// Optional library-membership (✓/＋) toggle column.
		if (libToggleCol >= 0) {
			TableColumn lt = jtable.getColumnModel().getColumn(libToggleCol);
			lt.setCellRenderer(new LibraryToggleRenderer());
			lt.setResizable(false);
			lt.setMinWidth(0);
			lt.setMaxWidth(LIB_TOGGLE_COL_WIDTH);
			lt.setPreferredWidth(LIB_TOGGLE_COL_WIDTH);
		}

		scroll = new JScrollPane(jtable);
		// Non-UIResource empty border so the Nimbus reinstall on theme change
		// doesn't re-install a default scroll-pane border.
		scroll.setBorder(BorderFactory.createEmptyBorder());
		scroll.setOpaque(true);
		scroll.getViewport().setOpaque(true);
		add(scroll, BorderLayout.CENTER);

		// Drag-and-drop: every music list can export its rows so they can be dropped
		// into a playlist (sidebar node or an open playlist view). The import side —
		// reorder within, or add dropped refs — is opt-in per view via
		// setReorderHandler / setAddHandler.
		jtable.setDragEnabled(true);
		jtable.setTransferHandler(new MusicTableTransferHandler());

		updateColors();
		ThemeManager.addChangeListener(this::updateColors);
		// Repaint so the now-playing row highlight follows the active track.
		PlayQueue.getInstance().addChangeListener(jtable::repaint);
		// Keep the managed columns (and grouped view) in sync automatically whenever
		// the model changes — including async / out-of-band fills — so callers never
		// have to remember to call refresh().
		model.addTableModelListener(e -> scheduleManagedSync());
	}

	// ── Public API ──────────────────────────────────────────────────────────────

	/** The table model to fill; pad-tolerant of the caller's own column count. */
	public DefaultTableModel getModel() { return model; }

	/** Remove every row, leaving the columns intact. */
	public void clear() { model.setRowCount(0); }

	/**
	 * Append a row whose values are bound to columns by key (see {@link #row()}).
	 * Values land in their column regardless of the order set, columns left unset
	 * are blank, and keys with no matching column are ignored — so the same call
	 * works whatever order/subset of columns this table was built with.
	 */
	public void addRow(Row row) {
		Object[] rowData = new Object[model.getColumnCount()];
		for (Map.Entry<String, Object> e : row.values.entrySet()) {
			Integer i = indexByKey.get(e.getKey());
			if (i != null)
				rowData[i] = e.getValue();
		}
		model.addRow(rowData);
	}

	/** Model column index for a column key, or -1 if this table has no such column. */
	public int columnIndex(String key) {
		Integer i = indexByKey.get(key);
		return i != null ? i : -1;
	}

	/** The {@link Column#key()} for a model column index, or {@code null} if out of range. */
	public String keyForColumn(int modelColumn) {
		return (modelColumn >= 0 && modelColumn < keyByIndex.length) ? keyByIndex[modelColumn] : null;
	}

	/** The underlying table (for selection queries and repaints). */
	public Table getTable() { return table; }

	/** The selected model row, or -1. */
	public int getSelectedRow() { return jtable.getSelectedRow(); }

	/** Select the given model row (no-op if out of range). */
	public void selectRow(int row) {
		if (row >= 0 && row < model.getRowCount()) jtable.setRowSelectionInterval(row, row);
	}

	/**
	 * Enable row drag-and-drop reordering of the flat table, routing each drop to
	 * {@code handler}; pass {@code null} to disable. Reordering is suppressed while
	 * the grouped presentation is showing. The handler is expected to apply the move
	 * and refresh the model (e.g. via the playlist service + its change event).
	 */
	public void setReorderHandler(ReorderHandler handler) {
		this.reorderHandler = handler;
		if (handler != null) jtable.setDropMode(DropMode.INSERT_ROWS);
	}

	/**
	 * Accept {@link PlayableRef}s dragged out of any music list and dropped onto this
	 * table, routing each to {@code handler} (e.g. add it to the playlist shown here);
	 * pass {@code null} to disable. Independent of {@link #setReorderHandler}: a view
	 * may enable both, in which case a drag originating from this same table reorders
	 * while a drag from elsewhere adds.
	 */
	public void setAddHandler(AddHandler handler) {
		this.addHandler = handler;
		if (handler != null) jtable.setDropMode(DropMode.INSERT_ROWS);
	}

	/**
	 * Re-sync the managed columns (and the grouped view) with the current model
	 * <em>immediately</em>. This now happens automatically whenever the model
	 * changes — a {@code TableModelListener} registered in the constructor covers
	 * sync, async and out-of-band fills — so callers no longer need to invoke it;
	 * it remains public only for a synchronous sync when one is specifically wanted.
	 */
	public void refresh() {
		syncManaged();
	}

	/** Coalesce a burst of model changes into a single managed sync on the EDT. */
	private void scheduleManagedSync() {
		if (managedSyncPending)
			return;
		managedSyncPending = true;
		SwingUtilities.invokeLater(() -> { managedSyncPending = false; syncManaged(); });
	}

	/**
	 * Size the managed columns (Buy/Stream, library-toggle) for the current rows,
	 * hiding them when no row is playable, and rebuild the grouped view if showing.
	 */
	private void syncManaged() {
		boolean any = false;
		for (int r = 0; r < model.getRowCount(); r++) {
			if (source.playRequestAt(r) != null) { any = true; break; }
		}
		sizeColumn(buyStreamCol, any ? BUY_STREAM_COL_WIDTH : 0);
		if (libToggleCol >= 0) sizeColumn(libToggleCol, any ? LIB_TOGGLE_COL_WIDTH : 0);
		if (grouped)
			rebuildGroups();
	}

	/** The grouping options offered by the source (cached after first read). */
	public List<Grouping> groupings() {
		if (cachedGroupings == null)
			cachedGroupings = new ArrayList<>(source.groupings());
		return cachedGroupings;
	}

	public boolean isGrouped() { return grouped; }

	/** Choose the active grouping; rebuilds immediately when grouped. */
	public void setGrouping(Grouping g) {
		currentGrouping = g;
		if (grouped)
			rebuildGroups();
	}

	/** Switch between the flat table and the grouped sections. */
	public void setGrouped(boolean on) {
		grouped = on;
		if (on) {
			if (groupedPanel == null)
				groupedPanel = new GroupedListPanel();
			if (currentGrouping == null && !groupings().isEmpty())
				currentGrouping = groupings().get(0);
			rebuildGroups();
			scroll.setViewportView(groupedPanel.getComponent());
		} else {
			scroll.setViewportView(jtable);
		}
		scroll.revalidate();
		scroll.repaint();
	}

	// ── Activation & context menu ────────────────────────────────────────────────

	/**
	 * Turn the visible rows into the play queue and start at {@code row}. Every
	 * playable row (per {@link Source#queueItemAt}) becomes a queue entry; if the
	 * source produces none, falls back to {@link Source#onActivate(int)}.
	 */
	private void activate(int row) {
		List<PlayQueueItem> queue = new ArrayList<>();
		int startIndex = -1;
		for (int i = 0; i < model.getRowCount(); i++) {
			PlayQueueItem item = source.queueItemAt(i);
			if (item == null)
				continue;
			if (i == row)
				startIndex = queue.size();
			queue.add(item);
		}
		if (startIndex >= 0) {
			PlayQueue.getInstance().setQueueAndPlay(queue, startIndex);
		} else {
			source.onActivate(row);
		}
	}

	/**
	 * Pop the row context menu ("Play with…") on a platform popup trigger, when the
	 * row under the cursor is playable.
	 */
	private void maybeShowRowMenu(MouseEvent e) {
		if (!e.isPopupTrigger())
			return;
		int row = jtable.rowAtPoint(e.getPoint());
		if (row < 0)
			return;
		jtable.setRowSelectionInterval(row, row);
		PlayRequest req = source.playRequestAt(row);
		if (req == null)
			return;
		JPopupMenu menu = new JPopupMenu();
		JMenuItem playWith = new JMenuItem("Play with…");
		playWith.addActionListener(a -> PlaybackCoordinator.resolveAndPlay(req, true));
		menu.add(playWith);
		menu.show(e.getComponent(), e.getX(), e.getY());
	}

	// ── Library-membership toggle (✓/＋) ──────────────────────────────────────────

	/** Toggle library membership for a clicked row; repaints so the glyph flips. */
	private boolean handleLibraryToggleClick(int row) {
		if (row < 0 || row >= model.getRowCount() || source.playRequestAt(row) == null) return false;
		source.toggleLibraryAt(row);
		jtable.repaint();
		return true;
	}

	/** Renders the library-membership glyph (✓ in library / ＋ to add), themed. */
	private final class LibraryToggleRenderer extends ThemedTableCellRenderer {
		private static final long serialVersionUID = 1L;
		@Override
		public Component getTableCellRendererComponent(JTable t, Object value, boolean sel,
				boolean focus, int row, int column) {
			super.getTableCellRendererComponent(t, value, sel, focus, row, column);
			boolean playable = source.playRequestAt(row) != null;
			boolean in = playable && source.inLibraryAt(row);
			setHorizontalAlignment(CENTER);
			setText(!playable ? "" : (in ? "✓" : "＋"));
			if (!sel) setForeground(in ? ThemeManager.getAccentBackgroundColor() : new Color(150, 150, 150));
			return this;
		}
	}

	// ── Buy/Stream split button ──────────────────────────────────────────────────

	/** Handle a click in the Buy/Stream cell: ▾ zone (or Buy-only) opens the menu,
	 *  the left part plays when streamable. Returns true if it consumed the click. */
	private boolean handleBuyStreamClick(int row, MouseEvent e) {
		PlayRequest req = source.playRequestAt(row);
		if (req == null) return false;
		TrackAvailability a = AvailabilityResolver.get().availabilityFor(req, jtable::repaint);
		if (a.local()) { activate(row); return true; }   // already have the file → play it
		Rectangle cell = jtable.getCellRect(row, buyStreamCol, false);
		boolean onArrow = (e.getX() - cell.x) >= cell.width - BUY_STREAM_ARROW_W - 4;
		if (onArrow || !a.hasStream()) {
			showBuyStreamMenu(row, req, a);
		} else {
			activate(row);   // default action: stream/play
		}
		return true;
	}

	private void showBuyStreamMenu(int row, PlayRequest req, TrackAvailability a) {
		JPopupMenu menu = new JPopupMenu();

		if (a.hasStream()) {
			JMenu stream = new JMenu("Stream");
			for (se.spacify.app.music.service.MusicService ms : a.streamServices()) {
				JMenuItem it = new JMenuItem("Play on " + ms.getName());
				it.addActionListener(x -> PlaybackCoordinator.playOn(ms, req));
				stream.add(it);
			}
			menu.add(stream);
		}

		JMenu buy = new JMenu("Buy");
		String query = ((req.title() == null ? "" : req.title())
			+ (req.artist() == null || req.artist().isBlank() ? "" : " " + req.artist())).trim();
		for (se.spacify.web.StoreCatalog.Store s : se.spacify.web.StoreCatalog.STORES) {
			JMenuItem it = new JMenuItem(s.name());
			it.addActionListener(x -> viewStack.navigate(s.searchUri(query)));
			buy.add(it);
		}
		menu.add(buy);

		menu.addSeparator();
		JMenuItem openWith = new JMenuItem("Open with…");
		openWith.addActionListener(x -> PlaybackCoordinator.resolveAndPlay(req, true));
		menu.add(openWith);

		Rectangle cell = jtable.getCellRect(row, buyStreamCol, false);
		menu.show(jtable, cell.x, cell.y + cell.height);
	}

	/**
	 * Cell renderer driven by {@link TrackAvailability}: a real {@link se.spacify.controls.Button}
	 * (so it uses the same skinned painting as every other button, via SpaceButtonUI)
	 * labelled "Stream ▾" / "Buy ▾"; or, when the track is matched locally, just a
	 * file icon and no button.
	 */
	private final class BuyStreamRenderer implements javax.swing.table.TableCellRenderer {
		private final BuyStreamButton button = new BuyStreamButton();
		// The cell host paints the row's zebra; the button sits inset and transparent
		// on top of it, so the striping shows around it. (The button alone would fill
		// the whole cell with its skinned face and hide the zebra.)
		private final JPanel          host = new JPanel(new BorderLayout());
		BuyStreamRenderer() {
			host.setBorder(BorderFactory.createEmptyBorder(3, 6, 3, 6));
			button.getComponent().setOpaque(false);
		}
		@Override
		public Component getTableCellRendererComponent(JTable t, Object value, boolean sel,
				boolean focus, int row, int column) {
			host.removeAll();
			host.setOpaque(true);
			host.setBackground(sel ? ThemeManager.getAccentBackgroundColor()
			                       : ThemedTableCellRenderer.rowBackground(t, row));
			PlayRequest req = source.playRequestAt(row);
			if (req == null) return host;   // empty cell — just the zebra
			TrackAvailability a = AvailabilityResolver.get().availabilityFor(req, jtable::repaint);
			if (a.local()) return host;   // already local — nothing to buy/stream, empty cell
			button.getComponent().setFont(t.getFont());
			button.getComponent().setText(!a.resolved() ? "…" : (a.hasStream() ? "Stream ▾" : "Buy ▾"));
			host.add(button.getComponent(), BorderLayout.CENTER);
			return host;
		}
	}

	/** A {@link se.spacify.controls.Button} usable as a detached table cell renderer —
	 *  falls back to the live window so the skin (SpaceButtonUI paint) still resolves. */
	private static final class BuyStreamButton extends se.spacify.controls.Button {
		BuyStreamButton() {
			super();
			component.setFocusable(false);
			component.setMargin(new java.awt.Insets(0, 0, 0, 0));
		}
	}

	// ── Row drag-and-drop: export refs, import reorders / adds ───────────────────

	/**
	 * Exports the dragged row as a {@link PlaylistDrag} (for dropping into a
	 * playlist) and, when reorder is enabled, the row index too; on import, a
	 * within-table drag (carrying a row index, reorder enabled) reorders, while any
	 * other ref drop is added via {@link #addHandler}.
	 */
	private final class MusicTableTransferHandler extends TransferHandler {
		private static final long serialVersionUID = 1L;

		@Override public int getSourceActions(JComponent c) { return COPY_OR_MOVE; }

		@Override protected Transferable createTransferable(JComponent c) {
			int row = jtable.getSelectedRow();
			if (row < 0) return null;
			PlayableRef ref = source.dragItemAt(row);
			// Only carry a row index when this table can reorder itself.
			Integer rowIndex = reorderHandler != null ? row : null;
			if (ref == null && rowIndex == null) return null;
			PlaylistDrag drag = ref != null ? new PlaylistDrag(ref, lastPressExpand) : null;
			return new MusicTransferable(drag, rowIndex);
		}

		@Override public boolean canImport(TransferSupport support) {
			if (!support.isDrop()) return false;
			if (reorderHandler != null && !grouped && support.isDataFlavorSupported(ROW_INDEX_FLAVOR))
				return true;
			return addHandler != null && support.isDataFlavorSupported(PLAYABLE_REF_FLAVOR);
		}

		@Override public boolean importData(TransferSupport support) {
			if (!canImport(support)) return false;
			int insert = ((JTable.DropLocation) support.getDropLocation()).getRow(); // 0..rowCount
			try {
				// A drag from this same table (row index present, reorder on) reorders;
				// anything else carrying a ref is an add from another list.
				if (reorderHandler != null && !grouped
						&& support.isDataFlavorSupported(ROW_INDEX_FLAVOR)) {
					int from = (Integer) support.getTransferable().getTransferData(ROW_INDEX_FLAVOR);
					int to = insert > from ? insert - 1 : insert;
					if (from < 0 || to < 0 || from == to) return false;
					reorderHandler.reorder(from, to);
					return true;
				}
				if (addHandler != null && support.isDataFlavorSupported(PLAYABLE_REF_FLAVOR)) {
					PlaylistDrag drag = (PlaylistDrag) support.getTransferable()
							.getTransferData(PLAYABLE_REF_FLAVOR);
					if (drag == null || drag.ref() == null) return false;
					addHandler.add(drag.ref(), drag.expand(), insert);
					return true;
				}
			} catch (Exception e) {
				return false;
			}
			return false;
		}
	}

	/** Carries the cross-view ref payload and, for within-table reorders, the row index. */
	private record MusicTransferable(PlaylistDrag drag, Integer rowIndex) implements Transferable {
		@Override public DataFlavor[] getTransferDataFlavors() {
			List<DataFlavor> flavors = new ArrayList<>(2);
			if (drag != null)     flavors.add(PLAYABLE_REF_FLAVOR);
			if (rowIndex != null) flavors.add(ROW_INDEX_FLAVOR);
			return flavors.toArray(new DataFlavor[0]);
		}
		@Override public boolean isDataFlavorSupported(DataFlavor f) {
			return (drag != null && PLAYABLE_REF_FLAVOR.equals(f))
				|| (rowIndex != null && ROW_INDEX_FLAVOR.equals(f));
		}
		@Override public Object getTransferData(DataFlavor f) {
			if (PLAYABLE_REF_FLAVOR.equals(f)) return drag;
			if (ROW_INDEX_FLAVOR.equals(f))    return rowIndex;
			return null;
		}
	}

	// ── Grouped presentation ─────────────────────────────────────────────────────

	/** Rebuild the grouped sections from the current model rows. */
	private void rebuildGroups() {
		if (groupedPanel == null)
			return;
		Grouping g = currentGrouping;
		if (g == null) {
			groupedPanel.setGroups(List.of());
			return;
		}
		// Preserve model row order within each section (rows already arrive in
		// album order), and section order by first appearance.
		Map<String, GroupBucket> buckets = new LinkedHashMap<>();
		for (int row = 0; row < model.getRowCount(); row++) {
			PlayQueueItem item = source.queueItemAt(row);
			if (item == null)
				continue;
			GroupRef maybe = g.groupOf(row);
			final GroupRef ref = maybe != null ? maybe : new GroupRef("none", "Unknown", "");
			GroupBucket bucket = buckets.computeIfAbsent(ref.key(), k -> new GroupBucket(ref));
			final int r = row;
			bucket.items.add(new GroupedListPanel.Item(
					item.getName(), item.getArtists(), fmtDuration(item.getDurationMs()),
					item.getKey(), () -> activate(r)));
		}
		List<GroupedListPanel.Group> groups = new ArrayList<>();
		for (GroupBucket bucket : buckets.values()) {
			Image image = GroupAvatar.of(bucket.ref.key(), bucket.ref.title(), 64);
			groups.add(new GroupedListPanel.Group(
					image, bucket.ref.title(), bucket.ref.subtitle(), bucket.items));
		}
		groupedPanel.setGroups(groups);
	}

	private static final class GroupBucket {
		final GroupRef ref;
		final List<GroupedListPanel.Item> items = new ArrayList<>();

		GroupBucket(GroupRef ref) {
			this.ref = ref;
		}
	}

	// ── Helpers ──────────────────────────────────────────────────────────────────

	private void sizeColumn(int index, int width) {
		TableColumn col = jtable.getColumnModel().getColumn(index);
		col.setMinWidth(0);
		col.setMaxWidth(width);
		col.setPreferredWidth(width);
	}

	/** Format a millisecond duration as {@code m:ss} ("" for non-positive). */
	public static String fmtDuration(long ms) {
		if (ms <= 0)
			return "";
		long s = ms / 1000;
		return String.format("%d:%02d", s / 60, s % 60);
	}

	private void updateColors() {
		Color bg = ThemeManager.getBackground();
		Color fg = ThemeManager.getForeground();
		Color grid = ThemeManager.getGridColor();

		jtable.setBackground(bg);
		jtable.setForeground(fg);
		jtable.setGridColor(grid);
		scroll.setBackground(bg);
		scroll.getViewport().setBackground(bg);
		jtable.repaint();
	}
}
