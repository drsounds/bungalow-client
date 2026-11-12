package se.bungalow.app.library.views;
import se.bungalow.app.music.controls.MusicTable;
import se.bungalow.app.music.controls.MusicTable.GroupRef;
import se.bungalow.app.music.controls.MusicTable.Grouping;
import se.bungalow.app.music.views.AbstractMusicListView;

import se.bungalow.controls.ToolButton;
import se.bungalow.db.DatabaseManager;
import se.bungalow.db.LibraryRepository;
import se.bungalow.app.music.model.Artist;
import se.bungalow.app.music.model.MusicRelease;
import se.bungalow.app.music.model.Recording;
import se.bungalow.app.music.model.Track;
import se.bungalow.library.LibraryEvents;
import se.bungalow.navigation.ViewStack;
import se.bungalow.service.media.PlaybackCoordinator;

import se.bungalow.service.media.PlayQueueItem;

import javax.swing.*;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Detail page for a single artist, reached from the sidebar or the Artists
 * list via {@code bungalow:library:artist:<id>}. Lists the artist's recordings
 * and the albums each appears on, optionally grouped by release; double-click
 * resolves and plays across Services. "Edit Artist…" renames the artist.
 */
public class ArtistDetailView extends AbstractMusicListView {

    public ArtistDetailView(ViewStack viewStack) {
        super(viewStack);
    }
    private static final Pattern URI = Pattern.compile("bungalow:library:artist:(\\d+)");

    /** The page URI for an artist. */
    public static String uriFor(Artist a) { return "bungalow:library:artist:" + a.getId(); }

    private final List<Recording> rows = new ArrayList<>();
    private int artistId = -1;

    private static final GroupRef NO_RELEASE = new GroupRef("release:none", "No release", "");

    @Override protected boolean isEditable() { return false; }

    @Override protected boolean supportsGrouping() { return true; }

    @Override protected List<Grouping> groupings() { return List.of(byRelease); }

    /** Group recordings by the first release they appear on. */
    private final Grouping byRelease = new Grouping() {
        @Override public String name() { return "Release"; }
        @Override public GroupRef groupOf(int row) {
            MusicRelease r = firstRelease(rows.get(row));
            if (r == null) return NO_RELEASE;
            return new GroupRef("release:" + r.getId(), r.getName(), r.getReleaseDate() != null ? r.getReleaseDate() : "");
        }
    };

    @Override protected List<MusicTable.Column> getColumns() {
        return List.of(
            column("recording", "Recording"),
            column("album",     "Album"),
            column("artists",   "Artists"));
    }

    @Override
    public void navigate(String uri) {
        Matcher m = URI.matcher(uri);
        artistId = m.matches() ? Integer.parseInt(m.group(1)) : -1;
    }

    @Override
    protected void reload() {
        rows.clear();
        musicTable.clear();
        if (artistId < 0) { setHeader(null); return; }
        try {
            Artist artist = DatabaseManager.getInstance().dao(Artist.class).queryForId(artistId);
            if (artist == null) { setHeader("Artist not found"); return; }
            setHeader(artist.getName());

            for (Recording rec : LibraryRepository.recordingsForArtist(artist)) {
                rows.add(rec);
                addRow(row()
                    .set("recording", rec.getName())
                    .set("album",     LibraryRepository.albumForRecording(rec))
                    .set("artists",   LibraryRepository.artistNamesForRecording(rec)));
            }
        } catch (Exception e) {
            showError(e);
        }
    }

    @Override
    protected PlayQueueItem queueItemAt(int row) {
        Recording rec = rows.get(row);
        return new PlayQueueItem(rec.getPlayUri(), rec.getName(),
                LibraryRepository.artistNamesForRecording(rec), rec.getDurationMs(), () -> {
            if (!PlaybackCoordinator.play(rec.getIsrc(), rec.getName(),
                    LibraryRepository.primaryArtistForRecording(rec))) {
                PlaybackCoordinator.playUri(rec.getPlayUri());
            }
        });
    }

    /** "Go to Recording", then one "Go to Release" per album the recording is on. */
    @Override
    protected List<MusicTable.RowAction> rowActionsAt(int row) {
        Recording rec = rows.get(row);
        List<MusicTable.RowAction> out = new ArrayList<>();
        out.add(goTo("Go to Recording", RecordingDetailView.uriFor(rec)));
        try {
            for (Track t : LibraryRepository.tracksForRecording(rec))
                if (t.getRelease() != null)
                    out.add(goTo("Go to Release: " + t.getRelease().getName(), ReleaseDetailView.uriFor(t.getRelease())));
        } catch (Exception ignored) {}
        return out;
    }

    @Override
    protected JComponent toolbarAccessory() {
        ToolButton edit = new ToolButton("Edit Artist…");
        edit.getComponent().addActionListener(e -> editArtist());
        return edit.getComponent();
    }

    private void editArtist() {
        try {
            Artist a = DatabaseManager.getInstance().dao(Artist.class).queryForId(artistId);
            if (a == null) return;
            JTextField name = new JTextField(a.getName());
            JTextField isni = new JTextField(a.getIsni());
            JTextField mbid = new JTextField(a.getMbid());
            if (!FormDialog.show(getComponent(), "Edit Artist",
                    new String[]{"Name", "ISNI", "MBID"},
                    new JComponent[]{name, isni, mbid})) return;
            if (name.getText().isBlank()) return;
            a.setName(name.getText().trim());
            a.setIsni(blankToNull(isni.getText()));
            a.setMbid(blankToNull(mbid.getText()));
            DatabaseManager.getInstance().dao(Artist.class).update(a);
            reloadAndRegroup();
            LibraryEvents.fireChanged();
        } catch (Exception e) {
            showError(e);
        }
    }

    private static MusicRelease firstRelease(Recording rec) {
        try {
            for (Track t : LibraryRepository.tracksForRecording(rec))
                if (t.getRelease() != null) return t.getRelease();
        } catch (Exception ignored) {}
        return null;
    }

    private static String blankToNull(String s) {
        return (s == null || s.isBlank()) ? null : s.trim();
    }

    @Override public boolean acceptsUri(String uri) { return uri != null && URI.matcher(uri).matches(); }
    @Override public String getName() { return "Artist"; }
}
