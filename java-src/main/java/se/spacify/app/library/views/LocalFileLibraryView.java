package se.spacify.app.library.views;
import se.spacify.app.music.controls.MusicTable;
import se.spacify.app.music.views.AbstractMusicListView;

import se.spacify.db.DatabaseManager;
import se.spacify.app.localmusic.model.LocalFile;
import se.spacify.navigation.ViewStack;
import se.spacify.service.media.PlaybackCoordinator;

import se.spacify.service.media.PlayQueueItem;

import javax.swing.*;
import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * Library view listing local audio files (name, artist, release, ISRC, path)
 * with full CRUD. Double-clicking a row plays it: it first tries to resolve the
 * ISRC across the registered music Services, falling back to direct local
 * playback.
 */
public class LocalFileLibraryView extends AbstractMusicListView {

    public LocalFileLibraryView(ViewStack viewStack) {
        super(viewStack);
    }
    private final List<LocalFile> rows = new ArrayList<>();

    @Override protected List<MusicTable.Column> getColumns() {
        return List.of(
            column("name",    "Name"),
            column("artist",  "Artist"),
            column("release", "Release"),
            column("isrc",    "ISRC"),
            column("file",    "File"));
    }

    @Override
    protected void reload() {
        rows.clear();
        musicTable.clear();
        try {
            for (LocalFile f : DatabaseManager.getInstance().dao(LocalFile.class).queryForAll()) {
                rows.add(f);
                addRow(row()
                    .set("name",    f.getName())
                    .set("artist",  f.getArtistName())
                    .set("release", f.getReleaseName())
                    .set("isrc",    f.getIsrc())
                    .set("file",    f.getFilePath()));
            }
        } catch (Exception e) {
            showError(e);
        }
    }

    @Override
    protected void onAdd() {
        JTextField name    = new JTextField();
        JTextField artist  = new JTextField();
        JTextField release = new JTextField();
        JTextField isrc    = new JTextField();
        JTextField path    = new JTextField();
        JButton browse = makeBrowseButton(path, name);
        if (!FormDialog.show(getComponent(), "New Local File",
                new String[]{"Name", "Artist", "Release", "ISRC", "File path", ""},
                new JComponent[]{name, artist, release, isrc, path, browse})) return;
        if (path.getText().isBlank()) return;
        try {
            LocalFile f = new LocalFile(
                name.getText().isBlank() ? new File(path.getText().trim()).getName() : name.getText().trim(),
                path.getText().trim());
            f.setArtistName(blankToNull(artist.getText()));
            f.setReleaseName(blankToNull(release.getText()));
            f.setIsrc(blankToNull(isrc.getText()));
            DatabaseManager.getInstance().dao(LocalFile.class).create(f);
        } catch (Exception e) {
            showError(e);
        }
    }

    @Override
    protected void onEdit(int row) {
        LocalFile f = rows.get(row);
        JTextField name    = new JTextField(f.getName());
        JTextField artist  = new JTextField(f.getArtistName());
        JTextField release = new JTextField(f.getReleaseName());
        JTextField isrc    = new JTextField(f.getIsrc());
        JTextField path    = new JTextField(f.getFilePath());
        JButton browse = makeBrowseButton(path, name);
        if (!FormDialog.show(getComponent(), "Edit Local File",
                new String[]{"Name", "Artist", "Release", "ISRC", "File path", ""},
                new JComponent[]{name, artist, release, isrc, path, browse})) return;
        if (path.getText().isBlank()) return;
        try {
            f.setName(name.getText().trim());
            f.setArtistName(blankToNull(artist.getText()));
            f.setReleaseName(blankToNull(release.getText()));
            f.setIsrc(blankToNull(isrc.getText()));
            f.setFilePath(path.getText().trim());
            DatabaseManager.getInstance().dao(LocalFile.class).update(f);
        } catch (Exception e) {
            showError(e);
        }
    }

    @Override
    protected void onDelete(int row) {
        LocalFile f = rows.get(row);
        if (!confirmDelete("local file \"" + f.getName() + "\"")) return;
        try {
            DatabaseManager.getInstance().dao(LocalFile.class).delete(f);
        } catch (Exception e) {
            showError(e);
        }
    }

    @Override
    protected PlayQueueItem queueItemAt(int row) {
        LocalFile f = rows.get(row);
        // A local file is its own playable — resolve straight to the file.
        return new PlayQueueItem(f.getPlayUri(), f.getName(), f.getArtistName(), f.getDurationMs(),
            () -> PlaybackCoordinator.playLocalFile(f));
    }

    private JButton makeBrowseButton(JTextField path, JTextField name) {
        JButton browse = new JButton("Browse…");
        browse.addActionListener(e -> {
            JFileChooser chooser = new JFileChooser();
            if (!path.getText().isBlank()) chooser.setSelectedFile(new File(path.getText().trim()));
            if (chooser.showOpenDialog(getComponent()) == JFileChooser.APPROVE_OPTION) {
                File chosen = chooser.getSelectedFile();
                path.setText(chosen.getAbsolutePath());
                if (name.getText().isBlank()) name.setText(chosen.getName());
            }
        });
        return browse;
    }

    private static String blankToNull(String s) {
        return (s == null || s.isBlank()) ? null : s.trim();
    }

    @Override public boolean acceptsUri(String uri) { return "spacify:library:local".equals(uri); }
    @Override public String getName() { return "Local Files"; }
}
