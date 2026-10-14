package se.spacify.app.library.views;
import se.spacify.app.music.controls.MusicTable;
import se.spacify.app.music.views.AbstractMusicListView;

import se.spacify.db.DatabaseManager;
import se.spacify.db.LibraryRepository;
import se.spacify.app.music.model.Recording;
import se.spacify.app.music.model.RecordingCreatorCredit;
import se.spacify.navigation.ViewStack;
import se.spacify.service.media.PlayRequest;

import javax.swing.*;
import java.util.ArrayList;
import java.util.List;

/** Library view listing recordings (name, artists) with full CRUD. */
public class RecordingsLibraryView extends AbstractMusicListView {

    public RecordingsLibraryView(ViewStack viewStack) {
        super(viewStack);
        //TODO Auto-generated constructor stub
    }
    private final List<Recording> rows = new ArrayList<>();

    @Override protected List<MusicTable.Column> getColumns() {
        return List.of(
            column("name",     "Name"),
            column("artists",  "Artists"),
            column("isrc",     "ISRC"),
            column("duration", "Duration"));
    }

    @Override
    protected void reload() {
        rows.clear();
        musicTable.clear();
        try {
            for (Recording r : DatabaseManager.getInstance().dao(Recording.class).queryForAll()) {
                rows.add(r);
                addRow(row()
                    .set("name",     r.getName())
                    .set("artists",  LibraryRepository.artistNamesForRecording(r))
                    .set("isrc",     r.getIsrc())
                    .set("duration", fmtDuration(r.getDurationMs())));
            }
        } catch (Exception e) {
            showError(e);
        }
    }

    @Override
    protected void onAdd() {
        JTextField title    = new JTextField();
        JTextField artists  = new JTextField();
        JTextField isrc     = new JTextField();
        JTextField duration = new JTextField();
        JTextField filePath = new JTextField();
        if (!FormDialog.show(getComponent(), "New Recording",
                new String[]{"Title", "Artists (comma-separated)", "ISRC", "Duration (m:ss)", "File path"},
                new JComponent[]{title, artists, isrc, duration, filePath})) return;
        if (title.getText().isBlank()) return;
        try {
            Recording r = new Recording(title.getText().trim());
            r.setIsrc(blankToNull(isrc.getText()));
            r.setDurationMs(parseDuration(duration.getText()));
            DatabaseManager.getInstance().dao(Recording.class).create(r);
            LibraryRepository.setFilePathForRecording(r, blankToNull(filePath.getText()));
            LibraryRepository.setRecordingArtists(r, LibraryRepository.parseArtistNames(artists.getText()));
        } catch (Exception e) {
            showError(e);
        }
    }

    @Override
    protected void onEdit(int row) {
        Recording r = rows.get(row);
        JTextField title    = new JTextField(r.getName());
        JTextField artists  = new JTextField(LibraryRepository.artistNamesForRecording(r));
        JTextField isrc     = new JTextField(r.getIsrc());
        JTextField duration = new JTextField(fmtDuration(r.getDurationMs()));
        JTextField filePath = new JTextField(LibraryRepository.filePathForRecording(r));
        if (!FormDialog.show(getComponent(), "Edit Recording",
                new String[]{"Title", "Artists (comma-separated)", "ISRC", "Duration (m:ss)", "File path"},
                new JComponent[]{title, artists, isrc, duration, filePath})) return;
        if (title.getText().isBlank()) return;
        try {
            r.setName(title.getText().trim());
            r.setIsrc(blankToNull(isrc.getText()));
            r.setDurationMs(parseDuration(duration.getText()));
            DatabaseManager.getInstance().dao(Recording.class).update(r);
            LibraryRepository.setFilePathForRecording(r, blankToNull(filePath.getText()));
            LibraryRepository.setRecordingArtists(r, LibraryRepository.parseArtistNames(artists.getText()));
        } catch (Exception e) {
            showError(e);
        }
    }

    @Override
    protected void onDelete(int row) {
        Recording r = rows.get(row);
        if (!confirmDelete("recording \"" + r.getName() + "\"")) return;
        try {
            DatabaseManager.getInstance().dao(RecordingCreatorCredit.class).delete(
                DatabaseManager.getInstance().dao(RecordingCreatorCredit.class).queryForEq("recording_id", r.getId()));
            DatabaseManager.getInstance().dao(Recording.class).delete(r);
        } catch (Exception e) {
            showError(e);
        }
    }

    @Override
    protected PlayRequest playRequestAt(int row) {
        Recording r = rows.get(row);
        // No local Track here; the saved "Play with…" pick is keyed by ISRC/URI.
        return new PlayRequest(null, r.getIsrc(), r.getName(),
                LibraryRepository.primaryArtistForRecording(r), r.getPlayUri(), r.getDurationMs());
    }

    private static String blankToNull(String s) {
        return (s == null || s.isBlank()) ? null : s.trim();
    }

    @Override public boolean acceptsUri(String uri) { return "spacify:library:recordings".equals(uri); }
    @Override public String getName() { return "Recordings"; }
}
