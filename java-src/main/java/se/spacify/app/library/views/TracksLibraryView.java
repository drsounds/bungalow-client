package se.spacify.app.library.views;
import se.spacify.app.music.controls.MusicTable;
import se.spacify.app.music.controls.MusicTable.GroupRef;
import se.spacify.app.music.controls.MusicTable.Grouping;
import se.spacify.app.music.views.AbstractMusicListView;
import se.spacify.db.DatabaseManager;
import se.spacify.db.LibraryRepository;
import se.spacify.app.music.model.Playable;
import se.spacify.app.music.model.Recording;
import se.spacify.app.music.model.MusicRelease;
import se.spacify.app.music.model.Track;
import se.spacify.navigation.ViewStack;
import se.spacify.app.playlist.model.Playlist;
import se.spacify.app.playlist.service.PlaylistService;
import se.spacify.controls.ToolButton;
import se.spacify.service.media.PlayRequest;

import javax.swing.*;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Library view listing tracks joined to their recording (name, artists) and
 * release (album), with full CRUD. Also serves as the default library landing
 * view ({@code spacify:library}).
 */
public class TracksLibraryView extends AbstractMusicListView {

    public TracksLibraryView(ViewStack viewStack) {
        super(viewStack);
        //TODO Auto-generated constructor stub
    }

    private final List<Track> rows = new ArrayList<>();

    /** Track play-URI → the playlist it first appears in; rebuilt on reload. */
    private final Map<String, GroupRef> playlistByUri = new HashMap<>();

    private static final GroupRef NOT_IN_PLAYLIST =
        new GroupRef("playlist:none", "Not in a playlist", "");

    @Override protected List<MusicTable.Column> getColumns() {
        return List.of(
            column("number",    "#"),
            column("recording", "Recording"),
            column("artists",   "Artists"),
            column("album",     "Album"));
    }

    @Override protected boolean supportsGrouping() { return true; }

    @Override
    protected List<Grouping> groupings() {
        return List.of(byRelease, byPlaylist);
    }

    /** Group tracks by the album (release) they belong to. */
    private final Grouping byRelease = new Grouping() {
        @Override public String name() { return "Release"; }
        @Override public GroupRef groupOf(int row) {
            MusicRelease r = rows.get(row).getRelease();
            if (r == null) return new GroupRef("release:none", "Unknown release", "");
            return new GroupRef("release:" + r.getId(), r.getName(),
                    LibraryRepository.artistNamesForRelease(r));
        }
    };

    /** Group tracks by the playlist they appear in (see {@link #playlistByUri}). */
    private final Grouping byPlaylist = new Grouping() {
        @Override public String name() { return "Playlist"; }
        @Override public GroupRef groupOf(int row) {
            String uri = rows.get(row).getPlayUri();
            GroupRef ref = uri != null ? playlistByUri.get(uri) : null;
            return ref != null ? ref : NOT_IN_PLAYLIST;
        }
    };

    @Override
    protected void reload() {
        rows.clear();
        musicTable.clear();
        rebuildPlaylistIndex();
        try {
            // Present albums coherently: group by release title, then within each
            // album fall back to the canonical side/track-number ascending order.
            List<Track> tracks = new ArrayList<>(DatabaseManager.getInstance().dao(Track.class).queryForAll());
            tracks.sort(Comparator.comparing(
                    (Track t) -> t.getRelease() != null && t.getRelease().getName() != null
                            ? t.getRelease().getName() : "",
                    String.CASE_INSENSITIVE_ORDER)
                .thenComparing(LibraryRepository.ALBUM_ORDER));
            for (Track t : tracks) {
                rows.add(t);
                Recording rec = t.getRecording();
                addRow(row()
                    .set("number",    t.getTrackNumber())
                    .set("recording", rec != null ? rec.getName() : "")
                    .set("artists",   rec != null ? LibraryRepository.artistNamesForRecording(rec) : "")
                    .set("album",     t.getRelease() != null ? t.getRelease().getName() : ""));
            }
        } catch (Exception e) {
            showError(e);
        }
    }

    /**
     * Map every playlist item's play-URI to its playlist so the "Playlist"
     * grouping can place each track. A track in several playlists is attributed
     * to the first one encountered.
     */
    private void rebuildPlaylistIndex() {
        playlistByUri.clear();
        for (PlaylistService svc : getViewStack().getMainWindow().getServiceManager().getServices(PlaylistService.class)) {
            for (Playlist pl : svc.getPlaylists()) {
                GroupRef ref = new GroupRef("playlist:" + pl.getPublicId(), pl.getName(),
                        pl.getItems().size() + " tracks");
                for (Playable item : pl.getItems()) {
                    String uri = item.getPlayUri();
                    if (uri != null) playlistByUri.putIfAbsent(uri, ref);
                }
            }
        }
    }

    @Override
    protected void onAdd() {
        try {
            List<Recording> recordings = DatabaseManager.getInstance().dao(Recording.class).queryForAll();
            List<MusicRelease>   releases   = DatabaseManager.getInstance().dao(MusicRelease.class).queryForAll();
            if (recordings.isEmpty() || releases.isEmpty()) {
                JOptionPane.showMessageDialog(getComponent(),
                    "Add at least one recording and one release first.",
                    "Cannot add track", JOptionPane.INFORMATION_MESSAGE);
                return;
            }
            JTextField number = new JTextField();
            JTextField side   = new JTextField();
            JTextField duration = new JTextField();
            JComboBox<Recording> recCombo = new JComboBox<>(recordings.toArray(new Recording[0]));
            JComboBox<MusicRelease>   relCombo = new JComboBox<>(releases.toArray(new MusicRelease[0]));
            if (!FormDialog.show(getComponent(), "New Track",
                    new String[]{"Track number", "Side", "Duration (m:ss)", "Recording", "Release"},
                    new JComponent[]{number, side, duration, recCombo, relCombo})) return;

            Track t = new Track();
            t.setTrackNumber(parseInt(number.getText()));
            t.setSide(blankToNull(side.getText()));
            t.setDurationMs(parseDuration(duration.getText()));
            t.setRecording((Recording) recCombo.getSelectedItem());
            t.setRelease((MusicRelease) relCombo.getSelectedItem());
            DatabaseManager.getInstance().dao(Track.class).create(t);
        } catch (Exception e) {
            showError(e);
        }
    }

    @Override
    protected void onEdit(int row) {
        Track t = rows.get(row);
        try {
            List<Recording> recordings = DatabaseManager.getInstance().dao(Recording.class).queryForAll();
            List<MusicRelease>   releases   = DatabaseManager.getInstance().dao(MusicRelease.class).queryForAll();
            JTextField number = new JTextField(String.valueOf(t.getTrackNumber()));
            JTextField side   = new JTextField(t.getSide());
            JTextField duration = new JTextField(fmtDuration(t.getDurationMs()));
            JComboBox<Recording> recCombo = new JComboBox<>(recordings.toArray(new Recording[0]));
            JComboBox<MusicRelease>   relCombo = new JComboBox<>(releases.toArray(new MusicRelease[0]));
            selectById(recCombo, t.getRecording() != null ? t.getRecording().getId() : -1);
            selectReleaseById(relCombo, t.getRelease() != null ? t.getRelease().getId() : -1);
            if (!FormDialog.show(getComponent(), "Edit Track",
                    new String[]{"Track number", "Side", "Duration (m:ss)", "Recording", "Release"},
                    new JComponent[]{number, side, duration, recCombo, relCombo})) return;

            t.setTrackNumber(parseInt(number.getText()));
            t.setSide(blankToNull(side.getText()));
            t.setDurationMs(parseDuration(duration.getText()));
            t.setRecording((Recording) recCombo.getSelectedItem());
            t.setRelease((MusicRelease) relCombo.getSelectedItem());
            DatabaseManager.getInstance().dao(Track.class).update(t);
        } catch (Exception e) {
            showError(e);
        }
    }

    @Override
    protected void onDelete(int row) {
        Track t = rows.get(row);
        if (!confirmDelete("track #" + t.getTrackNumber())) return;
        try {
            DatabaseManager.getInstance().dao(Track.class).delete(t);
        } catch (Exception e) {
            showError(e);
        }
    }

    @Override
    protected PlayRequest playRequestAt(int row) {
        Track t = rows.get(row);
        Recording rec = t.getRecording();
        String isrc   = rec != null ? rec.getIsrc()  : null;
        String title  = rec != null ? rec.getName() : "";
        String artist = rec != null ? LibraryRepository.primaryArtistForRecording(rec) : "";
        // Carry the local Track so a remembered "Play with…" pick binds by FK.
        return new PlayRequest(t, isrc, title, artist, t.getPlayUri(), t.getDurationMs());
    }

    // ── Add to playlist ─────────────────────────────────────────────────────────

    @Override
    protected JComponent toolbarAccessory() {
        ToolButton add = new ToolButton("Add to playlist");
        add.getComponent().addActionListener(e -> addSelectedToPlaylist());
        return add.getComponent();
    }

    private void addSelectedToPlaylist() {
        int row = table.getSelectedRow();
        if (row < 0) {
            JOptionPane.showMessageDialog(getComponent(), "Select a track first.",
                    "Add to playlist", JOptionPane.INFORMATION_MESSAGE);
            return;
        }
        PlaylistService svc = editablePlaylistService();
        if (svc == null) {
            JOptionPane.showMessageDialog(getComponent(), "No editable playlist is available.",
                    "Add to playlist", JOptionPane.INFORMATION_MESSAGE);
            return;
        }
        Track t = rows.get(row);
        final String NEW = "＋ New playlist…";
        List<Object> options = new ArrayList<>(svc.getPlaylists());
        options.add(NEW);
        Object choice = JOptionPane.showInputDialog(getComponent(), "Add to playlist:", "Add to playlist",
                JOptionPane.PLAIN_MESSAGE, null, options.toArray(), options.get(0));
        if (choice == null) return;
        try {
            Playlist target;
            if (NEW.equals(choice)) {
                String name = JOptionPane.showInputDialog(getComponent(), "Playlist name:", "New Playlist",
                        JOptionPane.PLAIN_MESSAGE);
                if (name == null || name.isBlank()) return;
                target = svc.createPlaylist(name.trim());
            } else {
                target = (Playlist) choice;
            }
            svc.addToPlaylist(target.getPublicId(), t);
        } catch (Exception e) {
            showError(e);
        }
    }

    private PlaylistService editablePlaylistService() {
        for (PlaylistService svc : getViewStack().getMainWindow().getServiceManager().getServices(PlaylistService.class))
            if (svc.isEditable()) return svc;
        return null;
    }

    private static void selectById(JComboBox<Recording> combo, int id) {
        for (int i = 0; i < combo.getItemCount(); i++) {
            if (combo.getItemAt(i).getId() == id) { combo.setSelectedIndex(i); return; }
        }
    }

    private static void selectReleaseById(JComboBox<MusicRelease> combo, int id) {
        for (int i = 0; i < combo.getItemCount(); i++) {
            if (combo.getItemAt(i).getId() == id) { combo.setSelectedIndex(i); return; }
        }
    }

    private static int parseInt(String s) {
        try { return Integer.parseInt(s.trim()); } catch (Exception e) { return 0; }
    }

    private static String blankToNull(String s) {
        return (s == null || s.isBlank()) ? null : s.trim();
    }

    @Override public boolean acceptsUri(String uri) {
        return uri != null && uri.matches("spacify:library(:tracks)?");
    }
    @Override public String getName() { return "Tracks"; }
}
