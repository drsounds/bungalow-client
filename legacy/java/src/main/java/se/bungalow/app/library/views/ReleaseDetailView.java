package se.bungalow.app.library.views;
import se.bungalow.app.music.controls.MusicTable;
import se.bungalow.app.music.views.AbstractMusicListView;

import se.bungalow.db.DatabaseManager;
import se.bungalow.db.LibraryRepository;
import se.bungalow.app.music.model.Recording;
import se.bungalow.app.music.model.MusicRelease;
import se.bungalow.app.music.model.Track;
import se.bungalow.navigation.ViewStack;
import se.bungalow.service.media.PlayRequest;

import javax.swing.*;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Tracklist of a single release, reached from the sidebar or the Releases list
 * via {@code bungalow:library:release:<id>}. Double-click resolves and plays a
 * track across the registered music Services. Add puts a recording (existing,
 * or a new one by name) on the release as its next track; Edit changes a
 * track's position and duration; Delete takes the track off the release (the
 * recording stays in the library).
 */
public class ReleaseDetailView extends AbstractMusicListView {
    public ReleaseDetailView(ViewStack viewStack) {
        super(viewStack);
    }
    private static final Pattern URI = Pattern.compile("bungalow:library:release:(\\d+)");

    /** The page URI for a release. */
    public static String uriFor(MusicRelease r) { return "bungalow:library:release:" + r.getId(); }

    private final List<Track> rows = new ArrayList<>();
    private int releaseId = -1;

    @Override protected List<MusicTable.Column> getColumns() {
        return List.of(
            column("number",    "#"),
            column("recording", "Recording"),
            column("artists",   "Artists"),
            column("duration",  "Duration"));
    }

    @Override
    public void navigate(String uri) {
        Matcher m = URI.matcher(uri);
        releaseId = m.matches() ? Integer.parseInt(m.group(1)) : -1;
    }

    private MusicRelease release() throws Exception {
        return releaseId < 0 ? null : DatabaseManager.getInstance().dao(MusicRelease.class).queryForId(releaseId);
    }

    @Override
    protected void reload() {
        rows.clear();
        musicTable.clear();
        if (releaseId < 0) { setHeader(null); return; }
        try {
            MusicRelease release = release();
            if (release == null) { setHeader("Release not found"); return; }
            String artists = LibraryRepository.artistNamesForRelease(release);
            setHeader(artists.isBlank() ? release.getName() : release.getName() + " — " + artists);

            for (Track t : LibraryRepository.tracksForRelease(release)) {
                rows.add(t);
                Recording rec = t.getRecording();
                addRow(row()
                    .set("number",    t.getSide() != null ? t.getSide() + t.getTrackNumber() : t.getTrackNumber())
                    .set("recording", rec != null ? rec.getName() : "")
                    .set("artists",   rec != null ? LibraryRepository.artistNamesForRecording(rec) : "")
                    .set("duration",  fmtDuration(t.getDurationMs())));
            }
        } catch (Exception e) {
            showError(e);
        }
    }

    /** Put a recording on this release as its next track. */
    @Override
    protected void onAdd() {
        try {
            MusicRelease release = release();
            if (release == null) return;
            final String NEW = "＋ New recording…";
            List<Object> options = new ArrayList<>();
            options.add(NEW);
            options.addAll(DatabaseManager.getInstance().dao(Recording.class).queryForAll());
            JComboBox<Object> recCombo = new JComboBox<>(options.toArray());
            JTextField newName = new JTextField();
            JTextField number  = new JTextField(String.valueOf(LibraryRepository.nextTrackNumber(release)));
            JTextField side    = new JTextField();
            if (!FormDialog.show(getComponent(), "Add Track to " + release.getName(),
                    new String[]{"Recording", "New recording name", "Track number", "Side"},
                    new JComponent[]{recCombo, newName, number, side})) return;

            Recording rec;
            if (NEW.equals(recCombo.getSelectedItem())) {
                if (newName.getText().isBlank()) return;
                rec = new Recording(newName.getText().trim());
                DatabaseManager.getInstance().dao(Recording.class).create(rec);
                // A new recording is credited like the release it was added to.
                LibraryRepository.setRecordingArtists(rec,
                    LibraryRepository.parseArtistNames(LibraryRepository.artistNamesForRelease(release)));
            } else {
                rec = (Recording) recCombo.getSelectedItem();
            }

            Track t = new Track();
            t.setRecording(rec);
            t.setRelease(release);
            t.setTrackNumber(parseInt(number.getText()));
            t.setSide(blankToNull(side.getText()));
            DatabaseManager.getInstance().dao(Track.class).create(t);
        } catch (Exception e) {
            showError(e);
        }
    }

    @Override
    protected void onEdit(int row) {
        Track t = rows.get(row);
        JTextField number   = new JTextField(String.valueOf(t.getTrackNumber()));
        JTextField side     = new JTextField(t.getSide());
        JTextField duration = new JTextField(fmtDuration(t.getDurationMs()));
        if (!FormDialog.show(getComponent(), "Edit Track",
                new String[]{"Track number", "Side", "Duration on release (m:ss)"},
                new JComponent[]{number, side, duration})) return;
        try {
            t.setTrackNumber(parseInt(number.getText()));
            t.setSide(blankToNull(side.getText()));
            t.setDurationMs(parseDuration(duration.getText()));
            DatabaseManager.getInstance().dao(Track.class).update(t);
        } catch (Exception e) {
            showError(e);
        }
    }

    @Override
    protected void onDelete(int row) {
        Track t = rows.get(row);
        if (!confirmDelete("track \"" + t.getName() + "\" from this release")) return;
        try {
            LibraryRepository.deleteTrack(t);
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
        return new PlayRequest(t, isrc, title, artist, t.getPlayUri(), t.getDurationMs());
    }

    @Override
    protected List<MusicTable.RowAction> rowActionsAt(int row) {
        return TracksLibraryView.trackActions(this, rows.get(row), false); // already on the release
    }

    private static int parseInt(String s) {
        try { return Integer.parseInt(s.trim()); } catch (Exception e) { return 0; }
    }

    private static String blankToNull(String s) {
        return (s == null || s.isBlank()) ? null : s.trim();
    }

    @Override public boolean acceptsUri(String uri) { return uri != null && URI.matcher(uri).matches(); }
    @Override public String getName() { return "Release"; }
}
