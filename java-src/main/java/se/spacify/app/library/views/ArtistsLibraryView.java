package se.spacify.app.library.views;
import se.spacify.app.music.controls.MusicTable;
import se.spacify.app.music.views.AbstractMusicListView;

import se.spacify.db.DatabaseManager;
import se.spacify.app.music.model.Artist;
import se.spacify.navigation.ViewStack;

import javax.swing.*;
import java.util.ArrayList;
import java.util.List;

/** Library view listing artists (name, ISNI, MBID) with full CRUD. */
public class ArtistsLibraryView extends AbstractMusicListView {

    public ArtistsLibraryView(ViewStack viewStack) {
        super(viewStack);
        //TODO Auto-generated constructor stub
    }
    private final List<Artist> rows = new ArrayList<>();

    @Override protected List<MusicTable.Column> getColumns() {
        return List.of(
            column("name", "Name"),
            column("isni", "ISNI"),
            column("mbid", "MBID"));
    }

    @Override
    protected void reload() {
        rows.clear();
        musicTable.clear();
        try {
            for (Artist a : DatabaseManager.getInstance().dao(Artist.class).queryForAll()) {
                rows.add(a);
                addRow(row()
                    .set("name", a.getName())
                    .set("isni", a.getIsni())
                    .set("mbid", a.getMbid()));
            }
        } catch (Exception e) {
            showError(e);
        }
    }

    @Override
    protected void onAdd() {
        JTextField name = new JTextField();
        JTextField isni = new JTextField();
        JTextField mbid = new JTextField();
        if (!FormDialog.show(getComponent(), "New Artist",
                new String[]{"Name", "ISNI", "MBID"},
                new JComponent[]{name, isni, mbid})) return;
        if (name.getText().isBlank()) return;
        try {
            Artist a = new Artist(name.getText().trim());
            a.setIsni(blankToNull(isni.getText()));
            a.setMbid(blankToNull(mbid.getText()));
            DatabaseManager.getInstance().dao(Artist.class).create(a);
        } catch (Exception e) {
            showError(e);
        }
    }

    @Override
    protected void onEdit(int row) {
        Artist a = rows.get(row);
        JTextField name = new JTextField(a.getName());
        JTextField isni = new JTextField(a.getIsni());
        JTextField mbid = new JTextField(a.getMbid());
        if (!FormDialog.show(getComponent(), "Edit Artist",
                new String[]{"Name", "ISNI", "MBID"},
                new JComponent[]{name, isni, mbid})) return;
        if (name.getText().isBlank()) return;
        try {
            a.setName(name.getText().trim());
            a.setIsni(blankToNull(isni.getText()));
            a.setMbid(blankToNull(mbid.getText()));
            DatabaseManager.getInstance().dao(Artist.class).update(a);
        } catch (Exception e) {
            showError(e);
        }
    }

    @Override
    protected void onDelete(int row) {
        Artist a = rows.get(row);
        if (!confirmDelete("artist \"" + a.getName() + "\"")) return;
        try {
            DatabaseManager.getInstance().dao(Artist.class).delete(a);
        } catch (Exception e) {
            showError(e);
        }
    }

    private static String blankToNull(String s) {
        return (s == null || s.isBlank()) ? null : s.trim();
    }

    @Override public boolean acceptsUri(String uri) { return "spacify:library:artists".equals(uri); }
    @Override public String getName() { return "Artists"; }
}
