	package se.spacify.app.playlist.views;

import se.spacify.app.music.controls.MusicTable;
import se.spacify.app.music.views.AbstractMusicListView;
import se.spacify.app.playlist.PlaylistEvents;
import se.spacify.app.playlist.service.PlaylistItem;
import se.spacify.app.playlist.service.PlaylistService;
import se.spacify.controls.ToolBar;
import se.spacify.controls.ToolButton;
import se.spacify.app.music.model.Playable;
import se.spacify.app.music.model.PlayableRef;
import se.spacify.app.playlist.model.Playlist;
import se.spacify.navigation.ViewStack;
import se.spacify.service.media.PlayRequest;

import javax.swing.*;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.util.ArrayList;
import java.util.List;

/**
 * Shows a single playlist (navigated via {@code spacify:playlist:<uuid>}): its
 * ordered items in a {@link MusicTable} with play-on-activate, plus playlist-level
 * actions (rename, delete, remove item, reorder) wired to the owning
 * {@link PlaylistService}. The special URI {@code spacify:playlist:new} prompts
 * for a name, creates a playlist and navigates to it. Mutations elsewhere refresh
 * this view via {@link PlaylistEvents}.
 */
public class PlaylistView extends AbstractMusicListView {

    private static final String PREFIX = "spacify:playlist:";

    private final List<Playable> items = new ArrayList<>();
    private String currentId;
    private final Runnable onPlaylistsChanged = this::onExternalChange;

    public PlaylistView(ViewStack viewStack) {
        super(viewStack);
        PlaylistEvents.addListener(onPlaylistsChanged);
        musicTable.setReorderHandler(this::reorder);
        musicTable.setAddHandler(this::addDropped);
    }

    /**
     * Accept a {@link PlayableRef} dragged in from another list. A plain item is
     * inserted at the drop position; an expandable one (a release/playlist) adds
     * its one kind-tagged row by default, or — when the expand (Alt) modifier was
     * held — appends each of its children instead.
     */
    private void addDropped(PlayableRef ref, boolean expand, int index) {
        PlaylistService svc = ownerOf(currentId);
        if (svc == null || !svc.isEditable()) return;
        try {
            if (expand && ref.isExpandable()) {
                for (Playable child : ref.expansion()) svc.addToPlaylist(currentId, child);
            } else {
                int appendedAt = items.size();   // where the appended row will land
                svc.addToPlaylist(currentId, ref, ref.getKind() != null ? ref.getKind().id() : null);
                if (index >= 0 && index < appendedAt) svc.moveRow(currentId, appendedAt, index);
            }
        } catch (Exception e) {
            showError(e);
        }
    }

    /** Apply a drag-and-drop reorder, then keep the moved row selected. */
    private void reorder(int from, int to) {
        PlaylistService svc = ownerOf(currentId);
        if (svc == null || !svc.isEditable()) return;
        try {
            svc.moveRow(currentId, from, to);
            musicTable.selectRow(to);
        } catch (Exception e) {
            showError(e);
        }
    }

    // ── Catalogue CRUD toolbar off; we manage playlist-level actions instead ────

    @Override protected boolean isEditable() { return false; }

    @Override protected List<MusicTable.Column> getColumns() {
        return List.of(
            column("number",   "#"),
            column("title",    "Title"),
            column("artist",   "Artist"),
            column("duration", "Duration"),
            column("spacify:buy-stream", "Buy/Stream")
        );
    }

    @Override
    protected JComponent toolbarAccessory() {
        ToolBar bar = new ToolBar();
        ToolButton rename = new ToolButton("Rename");
        ToolButton delete = new ToolButton("Delete");
        ToolButton add    = new ToolButton("Add…");
        ToolButton remove = new ToolButton("Remove");
        ToolButton up     = new ToolButton("Up");
        ToolButton down   = new ToolButton("Down");
        rename.getComponent().addActionListener(e -> renamePlaylist());
        delete.getComponent().addActionListener(e -> deletePlaylist());
        add.getComponent().addActionListener(e -> addManual());
        remove.getComponent().addActionListener(e -> removeSelected());
        up.getComponent().addActionListener(e -> move(-1));
        down.getComponent().addActionListener(e -> move(+1));
        bar.add(rename);
        bar.add(delete);
        bar.getComponent().addSeparator();
        bar.add(add);
        bar.add(remove);
        bar.add(up);
        bar.add(down);
        return bar.getComponent();
    }

    // ── Navigation ──────────────────────────────────────────────────────────────

    @Override public boolean acceptsUri(String uri) {
        return uri != null && uri.startsWith(PREFIX);
    }

    @Override
    public void navigate(String uri) {
        if (uri == null || !uri.startsWith(PREFIX)) return;
        String id = uri.substring(PREFIX.length());
        if (id.equals("new")) { createPlaylist(); return; }
        currentId = id;
        reloadAndRegroup();
    }

    @Override public String getName() { return "Playlist"; }

    // ── Rendering ───────────────────────────────────────────────────────────────

    @Override
    protected void reload() {
        items.clear();
        musicTable.clear();
        if (currentId == null) { setHeader("Playlist"); return; }
        PlaylistService svc = ownerOf(currentId);
        Playlist pl = svc != null ? svc.getPlaylist(currentId) : null;
        if (pl == null) { setHeader("Playlist not found"); return; }
        setHeader(pl.getName());
        int n = 1;
        for (Playable item : pl.getItems()) {
            items.add(item);
            addRow(row()
                .set("number",   n++)
                .set("title",    item.getName())
                .set("artist",   artistOf(item))
                .set("duration", fmtDuration(item.getDurationMs())));
        }
    }

    @Override
    protected PlayRequest playRequestAt(int row) {
        if (row < 0 || row >= items.size()) return null;
        Playable it = items.get(row);
        // Recover the portable ISRC from a musik: content URI so the item resolves
        // by identifier across Services, not just by its stored title/artist.
        String isrc = se.spacify.app.music.net.MusikUri.isrcOf(it.getPlayUri());
        return new PlayRequest(null, isrc, it.getName(), artistOf(it), it.getPlayUri(), it.getDurationMs());
    }

    // ── Playlist-level actions ──────────────────────────────────────────────────

    private void createPlaylist() {
        PlaylistService svc = editableService();
        if (svc == null) { showError(new Exception("No editable playlist service available")); return; }
        String name = JOptionPane.showInputDialog(getComponent(), "Playlist name:", "New Playlist",
                JOptionPane.PLAIN_MESSAGE);
        if (name == null || name.isBlank()) return;
        try {
            Playlist pl = svc.createPlaylist(name.trim());
            getViewStack().navigate(PREFIX + pl.getPublicId());
        } catch (Exception e) { showError(e); }
    }

    private void renamePlaylist() {
        PlaylistService svc = ownerOf(currentId);
        if (svc == null || !svc.isEditable()) return;
        Playlist pl = svc.getPlaylist(currentId);
        String current = pl != null ? pl.getName() : "";
        Object input = JOptionPane.showInputDialog(getComponent(), "Playlist name:", "Rename Playlist",
                JOptionPane.PLAIN_MESSAGE, null, null, current);
        if (!(input instanceof String name) || name.isBlank()) return;
        try { svc.renamePlaylist(currentId, name.trim()); } catch (Exception e) { showError(e); }
    }

    private void deletePlaylist() {
        PlaylistService svc = ownerOf(currentId);
        if (svc == null || !svc.isEditable()) return;
        if (!confirmDelete("this playlist")) return;
        try {
            svc.deletePlaylist(currentId);
            currentId = null;
            getViewStack().navigate("spacify:library");
        } catch (Exception e) { showError(e); }
    }

    /**
     * Manually add an entry to the current playlist via a small form (Title +
     * Play URI required; Artist and Duration optional). Appends the item; the
     * {@link PlaylistService} persists it and fires {@link PlaylistEvents}, which
     * refreshes this view.
     */
    private void addManual() {
        PlaylistService svc = ownerOf(currentId);
        if (currentId == null || svc == null || !svc.isEditable()) {
            showError(new Exception("This playlist can't be edited."));
            return;
        }
        JTextField title    = new JTextField(24);
        JTextField artist   = new JTextField(24);
        JTextField uri      = new JTextField(24);
        JTextField duration = new JTextField(24);
        uri.putClientProperty("JTextField.placeholderText", "musik:… or spacify:…");
        duration.putClientProperty("JTextField.placeholderText", "m:ss");
        if (!form("Add to Playlist",
                new String[]{ "Title", "Artist", "Play URI", "Duration (m:ss)" },
                new JComponent[]{ title, artist, uri, duration })) {
            return;
        }
        String t = title.getText().trim();
        String u = uri.getText().trim();
        // A play URI is required: PlaylistRow.contentUri is NOT NULL, and it's what
        // makes the entry resolvable/playable.
        if (t.isEmpty() || u.isEmpty()) {
            showError(new Exception("A title and a play URI are required."));
            return;
        }
        PlaylistItem item = new PlaylistItem(u, t, blankToNull(artist.getText()),
                parseDuration(duration.getText()));
        try {
            svc.addToPlaylist(currentId, item);
        } catch (Exception e) {
            showError(e);
        }
    }

    /** Lay out labelled fields in an OK/Cancel modal; true if OK was pressed. */
    private boolean form(String title, String[] labels, JComponent[] fields) {
        JPanel panel = new JPanel(new GridBagLayout());
        GridBagConstraints c = new GridBagConstraints();
        c.insets = new Insets(4, 4, 4, 4);
        c.anchor = GridBagConstraints.WEST;
        for (int i = 0; i < labels.length; i++) {
            c.gridx = 0; c.gridy = i; c.weightx = 0; c.fill = GridBagConstraints.NONE;
            panel.add(new JLabel(labels[i]), c);
            c.gridx = 1; c.weightx = 1; c.fill = GridBagConstraints.HORIZONTAL;
            panel.add(fields[i], c);
        }
        return JOptionPane.showConfirmDialog(getComponent(), panel, title,
                JOptionPane.OK_CANCEL_OPTION, JOptionPane.PLAIN_MESSAGE) == JOptionPane.OK_OPTION;
    }

    private static String blankToNull(String s) {
        return (s == null || s.isBlank()) ? null : s.trim();
    }

    private void removeSelected() {
        int row = table.getSelectedRow();
        PlaylistService svc = ownerOf(currentId);
        if (row < 0 || svc == null || !svc.isEditable()) return;
        try { svc.removeFromPlaylist(currentId, row); } catch (Exception e) { showError(e); }
    }

    private void move(int delta) {
        int row = table.getSelectedRow();
        PlaylistService svc = ownerOf(currentId);
        if (row < 0 || svc == null || !svc.isEditable()) return;
        int target = row + delta;
        if (target < 0 || target >= items.size()) return;
        try {
            svc.moveRow(currentId, row, target);
            table.getSelectionModel().setSelectionInterval(target, target);
        } catch (Exception e) { showError(e); }
    }

    // ── Helpers ─────────────────────────────────────────────────────────────────

    /** Refresh only when a change concerns the playlist currently shown. */
    private void onExternalChange() {
        if (currentId != null) reloadAndRegroup();
    }

    private static String artistOf(Playable item) {
        return item instanceof PlaylistItem pi && pi.getArtist() != null ? pi.getArtist() : "";
    }

    private List<PlaylistService> services() {
        return getViewStack().getMainWindow().getServiceManager().getServices(PlaylistService.class);
    }

    /** The service that owns the playlist with the given id, or null. */
    private PlaylistService ownerOf(String id) {
        if (id == null) return null;
        for (PlaylistService svc : services()) {
            try { if (svc.getPlaylist(id) != null) return svc; } catch (Exception ignored) {}
        }
        return null;
    }

    /** The first editable playlist service (the local store), or null. */
    private PlaylistService editableService() {
        for (PlaylistService svc : services()) if (svc.isEditable()) return svc;
        return null;
    }
}
