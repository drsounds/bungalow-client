package se.spacify.app.music.views;

import se.spacify.controls.ToggleButton;
import se.spacify.controls.ToolBar;
import se.spacify.controls.ToolButton;
import se.spacify.library.LibraryEvents;
import se.spacify.navigation.View;
import se.spacify.navigation.ViewStack;
import se.spacify.app.library.views.LibraryScanAction;
import se.spacify.app.music.controls.MusicTable;
import se.spacify.app.music.model.PlayableKind;
import se.spacify.app.music.model.PlayableRef;
import se.spacify.service.media.PlaybackCoordinator;
import se.spacify.service.media.PlayQueueItem;
import se.spacify.service.media.PlayRequest;

import se.spacify.ui.theme.ThemeManager;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.util.List;

/**
 * Base for the data-backed music views. Provides the page chrome — an optional
 * header and a CRUD toolbar (Add / Edit / Delete / Scan / Refresh), plus an
 * optional grouped-view switch — wrapped around a reusable {@link MusicTable}.
 * Read-only views (see {@link #isEditable()}) keep just Refresh; mutating actions
 * broadcast via {@link LibraryEvents} so the sidebar stays in sync.
 *
 * <p>The table itself — the Buy/Stream column, the library-membership toggle,
 * play-queue activation, the "Play with…" menu and the grouped presentation — all
 * live in {@link MusicTable}, which other apps can embed directly without
 * subclassing this view. Subclasses here fill {@link #model} in {@link #reload()}
 * and answer the per-row hooks ({@link #playRequestAt}, etc.).
 */
public abstract class AbstractMusicListView extends View {

	protected final JLabel headerLabel;
	/** The reusable music table; subclasses normally interact via {@link #model}. */
	protected final MusicTable musicTable;
	/** The table model to fill in {@link #reload()} (owned by {@link #musicTable}). */
	protected final DefaultTableModel model;
	/** The underlying table (owned by {@link #musicTable}); for selection queries. */
	protected final javax.swing.JTable table;

	private ToolBar bottomToolbar;
	private ToolBar toolbar;
	private ToolButton refreshBtn;

	// ── Optional grouped presentation (see supportsGrouping/groupings) ──────────
	private ToggleButton groupToggle;
	private JComboBox<MusicTable.Grouping> groupingChooser;

	protected AbstractMusicListView(ViewStack viewStack) {
		super(viewStack);
		getComponent().setLayout(new BorderLayout(0, 0));
		getComponent().setOpaque(false);

		headerLabel = new JLabel();
		headerLabel.setFont(headerLabel.getFont().deriveFont(Font.BOLD, 18f));
		headerLabel.setBorder(BorderFactory.createEmptyBorder(4, 6, 4, 6));
		headerLabel.setVisible(false);

		// ── Reusable music table ──────────────────────────────────────────────────
		// getColumns()/showsLibraryToggle() are queried during construction and must
		// be constant; the SourceAdapter forwards the per-row hooks to this view.
		musicTable = new MusicTable(viewStack, getColumns(), showsLibraryToggle(), new SourceAdapter());
		model = musicTable.getModel();
		table = musicTable.getTable().getComponent();

		// ── CRUD toolbar ───────────────────────────────────────────────────────
		toolbar = new ToolBar();

		refreshBtn = new ToolButton("Refresh");
		refreshBtn.getComponent().addActionListener(e -> reloadAndRegroup());

		if (isEditable()) {
			ToolButton addBtn = new ToolButton("Add");
			ToolButton editBtn = new ToolButton("Edit");
			ToolButton deleteBtn = new ToolButton("Delete");
			ToolButton scanBtn = new ToolButton("Scan…");

			scanBtn.getComponent().addActionListener(e -> LibraryScanAction.run(getComponent(), () -> {
				reloadAndRegroup();
				LibraryEvents.fireChanged();
			}));
			addBtn.getComponent().addActionListener(e -> {
				onAdd();
				reloadAndRegroup();
				LibraryEvents.fireChanged();
			});
			editBtn.getComponent().addActionListener(e -> {
				int row = table.getSelectedRow();
				if (row >= 0) {
					onEdit(row);
					reloadAndRegroup();
					LibraryEvents.fireChanged();
				}
			});
			deleteBtn.getComponent().addActionListener(e -> {
				int row = table.getSelectedRow();
				if (row >= 0) {
					onDelete(row);
					reloadAndRegroup();
					LibraryEvents.fireChanged();
				}
			});

			toolbar.add(addBtn);
			toolbar.add(editBtn);
			toolbar.add(deleteBtn);
			toolbar.getComponent().addSeparator();
			toolbar.add(scanBtn);
		}
		toolbar.add(refreshBtn);

		// ── Optional grouped-view switch ────────────────────────────────────────
		// supportsGrouping() must be a constant (it's called mid-construction);
		// the actual Grouping list is read lazily from the table once built.
		if (supportsGrouping()) {
			groupingChooser = new JComboBox<>();
			groupingChooser.setRenderer(new DefaultListCellRenderer() {
				private static final long serialVersionUID = 1L;
				@Override
				public Component getListCellRendererComponent(JList<?> list, Object value, int index,
						boolean isSelected, boolean hasFocus) {
					super.getListCellRendererComponent(list, value, index, isSelected, hasFocus);
					setText(value instanceof MusicTable.Grouping g ? g.name() : "");
					return this;
				}
			});
			groupingChooser.setVisible(false);
			groupingChooser.addActionListener(e -> {
				if (musicTable.isGrouped())
					musicTable.setGrouping((MusicTable.Grouping) groupingChooser.getSelectedItem());
			});

			groupToggle = new ToggleButton("Grouped");
			groupToggle.getComponent().addActionListener(e -> setGrouped(groupToggle.getComponent().isSelected()));

			toolbar.getComponent().addSeparator();
			toolbar.add(groupToggle);
			toolbar.getComponent().add(groupingChooser);
		}

		// Subclass-contributed toolbar control (e.g. a catalogue search field).
		JComponent accessory = toolbarAccessory();
		if (accessory != null) {
			toolbar.getComponent().addSeparator();
			toolbar.getComponent().add(accessory);
		}

		JPanel north = new JPanel();
		north.setLayout(new BoxLayout(north, BoxLayout.PAGE_AXIS));
		north.setOpaque(false);
		north.add(toolbar.getComponent(), BorderLayout.CENTER);
		//add(headerLabel, BorderLayout.NORTH);

		getComponent().add(north, BorderLayout.NORTH);
		getComponent().add(musicTable, BorderLayout.CENTER);

		bottomToolbar = new ToolBar();
		bottomToolbar.getComponent().add(new JButton("Test"));
		add(bottomToolbar, BorderLayout.SOUTH);

		updateColors();
		ThemeManager.addChangeListener(this::updateColors);
	}

	/** Drives the table between flat and grouped, populating the chooser on demand. */
	private void setGrouped(boolean on) {
		if (groupingChooser != null) {
			groupingChooser.setVisible(on);
			if (on && groupingChooser.getItemCount() == 0)
				for (MusicTable.Grouping g : musicTable.groupings())
					groupingChooser.addItem(g);
			if (on)
				musicTable.setGrouping((MusicTable.Grouping) groupingChooser.getSelectedItem());
		}
		musicTable.setGrouped(on);
	}

	/** Sets the page header text; pass null/blank to hide it. */
	protected void setHeader(String text) {
		headerLabel.setText(text == null ? "" : text);
		headerLabel.setVisible(text != null && !text.isBlank());
	}

	/**
	 * Whether this view shows Add/Edit/Delete/Scan; read-only views return false.
	 */
	protected boolean isEditable() {
		return true;
	}

	/**
	 * Whether this view offers the optional grouped presentation (a "Grouped"
	 * toggle plus a grouping chooser). Must be constant — it is queried during
	 * construction. Views that return true must also override {@link #groupings()}.
	 */
	protected boolean supportsGrouping() {
		return false;
	}

	/**
	 * The grouping options offered when {@link #supportsGrouping()} is true; read
	 * lazily after construction. The first entry is the default selection.
	 */
	protected List<MusicTable.Grouping> groupings() {
		return List.of();
	}

	// ── Hooks for subclasses ────────────────────────────────────────────────────

	/**
	 * The view's columns (key + title); called once during construction (must be
	 * constant). Rows are filled by column key (see {@link #addRow}), so a later
	 * view can reorder or drop columns without changing its row-building code.
	 */
	protected abstract List<MusicTable.Column> getColumns();

	/** Convenience: a {@link MusicTable.Column} with the given key and header title. */
	protected static MusicTable.Column column(String key, String title) {
		return new MusicTable.Column(key, title);
	}

	/** Start a keyed row; fill with {@link MusicTable.Row#set} and pass to {@link #addRow}. */
	protected static MusicTable.Row row() {
		return MusicTable.row();
	}

	/** Append a keyed row to the table (values bound by column key, not position). */
	protected void addRow(MusicTable.Row row) {
		musicTable.addRow(row);
	}

	/** Clear and refill {@link #model} from the database. */
	protected abstract void reload();

	protected void onAdd() {
	}

	protected void onEdit(int row) {
	}

	protected void onDelete(int row) {
	}

	/** Invoked when a row is double-clicked and isn't otherwise playable. */
	protected void onActivate(int row) {
	}

	/** Invoked on a single click of a cell; {@code columnKey} is the column's key
	 *  (see {@link MusicTable.Column#key()}), not a position. Default does nothing. */
	protected void onCellClicked(int row, String columnKey) {
	}

	// ── Library-membership toggle (✓/＋) — consistent across track lists ──────────

	/** Whether this view shows the library-membership toggle column. Constant. */
	protected boolean showsLibraryToggle() { return false; }

	/** Whether the row's track is saved in the local library (drives ✓ vs ＋). */
	protected boolean inLibraryAt(int row) { return false; }

	/** Add/remove the row's track to/from the local library. */
	protected void toggleLibraryAt(int row) {}

	/**
	 * An optional component contributed to the right of the toolbar — e.g. a
	 * catalogue search field. Called once during construction; returning a freshly
	 * built component (not a re-initialised field) keeps the reference stable.
	 */
	protected JComponent toolbarAccessory() {
		return null;
	}

	/**
	 * Supply a {@link PlayRequest} for the given model row, or {@code null} if the
	 * row isn't playable. This is the preferred hook for metadata-resolved views:
	 * the default {@link #queueItemAt(int)} builds a queue entry from it whose
	 * playback runs through {@link PlaybackCoordinator#resolveAndPlay}, and the
	 * right-click "Play with…" menu uses it too. Views that play something concrete
	 * (a local file) may instead override {@link #queueItemAt(int)} directly.
	 */
	protected PlayRequest playRequestAt(int row) {
		return null;
	}

	/**
	 * Supply a {@link PlayQueueItem} for the given model row, or {@code null} if
	 * the row isn't playable. By default this derives from {@link #playRequestAt}
	 * (routing playback through the "Play with…" resolver); views that play a
	 * concrete target may override it. Views that produce neither fall back to
	 * {@link #onActivate(int)}.
	 */
	protected PlayQueueItem queueItemAt(int row) {
		PlayRequest req = playRequestAt(row);
		if (req == null)
			return null;
		return new PlayQueueItem(req.key(), req.title(), req.artist(), req.durationMs(),
				() -> PlaybackCoordinator.resolveAndPlay(req), req);
	}

	/**
	 * The kind-tagged {@link PlayableRef} dragged when this row is dropped into a
	 * playlist, or {@code null} if the row isn't draggable. The default derives a
	 * {@link PlayableKind#TRACK} reference from {@link #playRequestAt}; views whose
	 * rows are releases/playlists override this to tag the kind and supply the
	 * expansion added when the drop's expand (Alt) modifier is held.
	 */
	protected PlayableRef dragItemAt(int row) {
		PlayRequest req = playRequestAt(row);
		if (req == null)
			return null;
		String uri = req.key();
		return new PlayableRef(PlayableKind.fromUri(uri), uri,
				req.title(), req.artist(), req.durationMs(), List.of());
	}

	/** Repopulate the model, then refresh the table (managed columns + grouping). */
	protected void reloadAndRegroup() {
		reload();
		musicTable.refresh();
	}

	// ── Shared helpers ──────────────────────────────────────────────────────────

	protected static String fmtDuration(long ms) {
		return MusicTable.fmtDuration(ms);
	}

	/** Parse "m:ss" or a plain seconds value into milliseconds; 0 on failure. */
	protected static long parseDuration(String text) {
		if (text == null)
			return 0;
		String t = text.trim();
		if (t.isEmpty())
			return 0;
		try {
			if (t.contains(":")) {
				String[] p = t.split(":");
				return (Long.parseLong(p[0].trim()) * 60 + Long.parseLong(p[1].trim())) * 1000;
			}
			return (long) (Double.parseDouble(t) * 1000);
		} catch (NumberFormatException e) {
			return 0;
		}
	}

	protected void showError(Exception e) {
		JOptionPane.showMessageDialog(getComponent(), e.getMessage(), "Library error", JOptionPane.ERROR_MESSAGE);
	}

	protected boolean confirmDelete(String what) {
		return JOptionPane.showConfirmDialog(getComponent(), "Delete " + what + "?", "Confirm delete", JOptionPane.YES_NO_OPTION,
				JOptionPane.WARNING_MESSAGE) == JOptionPane.YES_OPTION;
	}

	private void updateColors() {
		headerLabel.setForeground(ThemeManager.getForeground());
	}

	/** Forwards this view's per-row hooks to the embedded {@link MusicTable}. */
	private final class SourceAdapter implements MusicTable.Source {
		@Override public PlayRequest playRequestAt(int row) { return AbstractMusicListView.this.playRequestAt(row); }
		@Override public PlayQueueItem queueItemAt(int row) { return AbstractMusicListView.this.queueItemAt(row); }
		@Override public PlayableRef dragItemAt(int row)    { return AbstractMusicListView.this.dragItemAt(row); }
		@Override public boolean inLibraryAt(int row)       { return AbstractMusicListView.this.inLibraryAt(row); }
		@Override public void toggleLibraryAt(int row)      { AbstractMusicListView.this.toggleLibraryAt(row); }
		@Override public void onActivate(int row)           { AbstractMusicListView.this.onActivate(row); }
		@Override public void onCellClicked(int row, String columnKey) { AbstractMusicListView.this.onCellClicked(row, columnKey); }
		@Override public List<MusicTable.Grouping> groupings() { return AbstractMusicListView.this.groupings(); }
	}

	// ── SPView ──────────────────────────────────────────────────────────────────

	@Override
	public void navigate(String uri) {
	}

	@Override
	public void onShow() {
		reloadAndRegroup();
	}
}
