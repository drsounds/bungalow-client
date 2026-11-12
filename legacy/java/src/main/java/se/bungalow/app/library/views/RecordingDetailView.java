package se.bungalow.app.library.views;
import se.bungalow.app.music.controls.MusicTable;
import se.bungalow.app.music.views.AbstractMusicListView;

import se.bungalow.controls.ToolButton;
import se.bungalow.db.DatabaseManager;
import se.bungalow.db.LibraryRepository;
import se.bungalow.app.music.model.Recording;
import se.bungalow.app.music.model.Track;
import se.bungalow.library.LibraryEvents;
import se.bungalow.navigation.ViewStack;
import se.bungalow.service.media.PlayRequest;

import javax.swing.*;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Detail page for a single recording, reached via
 * {@code bungalow:library:recording:<id>}: one row per release the recording
 * appears on (its tracks). Double-click plays; "Edit Recording…" edits the
 * recording's name, artists, ISRC and duration.
 */
public class RecordingDetailView extends AbstractMusicListView {

    public RecordingDetailView(ViewStack viewStack) {
        super(viewStack);
    }
    private static final Pattern URI = Pattern.compile("bungalow:library:recording:(\\d+)");

    /** The page URI for a recording. */
    public static String uriFor(Recording r) { return "bungalow:library:recording:" + r.getId(); }

    private final List<Track> rows = new ArrayList<>();
    private int recordingId = -1;

    @Override protected boolean isEditable() { return false; }

    @Override protected List<MusicTable.Column> getColumns() {
        return List.of(
            column("release",  "Release"),
            column("number",   "#"),
            column("date",     "Date"),
            column("duration", "Duration"));
    }

    @Override
    public void navigate(String uri) {
        Matcher m = URI.matcher(uri);
        recordingId = m.matches() ? Integer.parseInt(m.group(1)) : -1;
    }

    private Recording recording() throws Exception {
        return recordingId < 0 ? null : DatabaseManager.getInstance().dao(Recording.class).queryForId(recordingId);
    }

    @Override
    protected void reload() {
        rows.clear();
        musicTable.clear();
        if (recordingId < 0) { setHeader(null); return; }
        try {
            Recording rec = recording();
            if (rec == null) { setHeader("Recording not found"); return; }
            String artists = LibraryRepository.artistNamesForRecording(rec);
            setHeader(artists.isBlank() ? rec.getName() : rec.getName() + " — " + artists);

            for (Track t : LibraryRepository.tracksForRecording(rec)) {
                rows.add(t);
                addRow(row()
                    .set("release",  t.getRelease() != null ? t.getRelease().getName() : "")
                    .set("number",   t.getTrackNumber())
                    .set("date",     t.getRelease() != null ? t.getRelease().getReleaseDate() : "")
                    .set("duration", fmtDuration(t.getDurationMs())));
            }
        } catch (Exception e) {
            showError(e);
        }
    }

    @Override
    protected PlayRequest playRequestAt(int row) {
        Track t = rows.get(row);
        Recording rec = t.getRecording();
        return new PlayRequest(t, rec.getIsrc(), rec.getName(),
                LibraryRepository.primaryArtistForRecording(rec), t.getPlayUri(), t.getDurationMs());
    }

    @Override
    protected List<MusicTable.RowAction> rowActionsAt(int row) {
        Track t = rows.get(row);
        return t.getRelease() != null
            ? List.of(goTo("Go to Release", ReleaseDetailView.uriFor(t.getRelease())))
            : List.of();
    }

    @Override
    protected JComponent toolbarAccessory() {
        ToolButton edit = new ToolButton("Edit Recording…");
        edit.getComponent().addActionListener(e -> editRecording());
        return edit.getComponent();
    }

    private void editRecording() {
        try {
            Recording r = recording();
            if (r == null) return;
            JTextField name     = new JTextField(r.getName());
            JTextField artists  = new JTextField(LibraryRepository.artistNamesForRecording(r));
            JTextField isrc     = new JTextField(r.getIsrc());
            JTextField duration = new JTextField(fmtDuration(r.getDurationMs()));
            if (!FormDialog.show(getComponent(), "Edit Recording",
                    new String[]{"Name", "Artists (comma-separated)", "ISRC", "Duration (m:ss)"},
                    new JComponent[]{name, artists, isrc, duration})) return;
            if (name.getText().isBlank()) return;
            r.setName(name.getText().trim());
            r.setIsrc(isrc.getText().isBlank() ? null : isrc.getText().trim());
            r.setDurationMs(parseDuration(duration.getText()));
            DatabaseManager.getInstance().dao(Recording.class).update(r);
            LibraryRepository.setRecordingArtists(r, LibraryRepository.parseArtistNames(artists.getText()));
            reloadAndRegroup();
            LibraryEvents.fireChanged();
        } catch (Exception e) {
            showError(e);
        }
    }

    @Override public boolean acceptsUri(String uri) { return uri != null && URI.matcher(uri).matches(); }
    @Override public String getName() { return "Recording"; }
}
