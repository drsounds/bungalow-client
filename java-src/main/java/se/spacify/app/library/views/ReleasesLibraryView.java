package se.spacify.app.library.views;
import se.spacify.app.music.controls.MusicTable;
import se.spacify.app.music.views.AbstractMusicListView;

import se.spacify.db.DatabaseManager;
import se.spacify.db.LibraryRepository;
import se.spacify.app.music.model.MusicRelease;
import se.spacify.app.music.model.Playable;
import se.spacify.app.music.model.PlayableKind;
import se.spacify.app.music.model.PlayableRef;
import se.spacify.app.music.model.ReleaseCreatorCredit;
import se.spacify.app.music.model.Track;
import se.spacify.app.music.model.MusicRelease.ReleaseType;
import se.spacify.navigation.ViewStack;

import javax.swing.*;
import java.util.ArrayList;
import java.util.List;

/** Library view listing releases (name, artists, type, date) with full CRUD. */
public class ReleasesLibraryView extends AbstractMusicListView {

    public ReleasesLibraryView(ViewStack viewStack) {
        super(viewStack);
        //TODO Auto-generated constructor stub
    }
    private final List<MusicRelease> rows = new ArrayList<>();

    @Override protected List<MusicTable.Column> getColumns() {
        return List.of(
            column("name",    "Name"),
            column("artists", "Artists"),
            column("type",    "Type"),
            column("date",    "Date"));
    }

    @Override
    protected void reload() {
        rows.clear();
        musicTable.clear();
        try {
            for (MusicRelease r : DatabaseManager.getInstance().dao(MusicRelease.class).queryForAll()) {
                rows.add(r);
                addRow(row()
                    .set("name",    r.getName())
                    .set("artists", LibraryRepository.artistNamesForRelease(r))
                    .set("type",    r.getType() != null ? r.getType().name() : "")
                    .set("date",    r.getReleaseDate()));
            }
        } catch (Exception e) {
            showError(e);
        }
    }

    @Override
    protected void onAdd() {
        JTextField title   = new JTextField();
        JTextField artists = new JTextField();
        JComboBox<ReleaseType> type = new JComboBox<>(ReleaseType.values());
        JTextField date    = new JTextField();
        JTextField upc     = new JTextField();
        JTextField mbid    = new JTextField();
        if (!FormDialog.show(getComponent(), "New Release",
                new String[]{"Title", "Artists (comma-separated)", "Type", "Date (YYYY-MM-DD)", "UPC", "MBID"},
                new JComponent[]{title, artists, type, date, upc, mbid})) return;
        if (title.getText().isBlank()) return;
        try {
            MusicRelease r = new MusicRelease(title.getText().trim());
            r.setType((ReleaseType) type.getSelectedItem());
            r.setReleaseDate(blankToNull(date.getText()));
            r.setUpc(blankToNull(upc.getText()));
            r.setMbid(blankToNull(mbid.getText()));
            DatabaseManager.getInstance().dao(MusicRelease.class).create(r);
            LibraryRepository.setReleaseArtists(r, LibraryRepository.parseArtistNames(artists.getText()));
        } catch (Exception e) {
            showError(e);
        }
    }

    @Override
    protected void onEdit(int row) {
        MusicRelease r = rows.get(row);
        JTextField title   = new JTextField(r.getName());
        JTextField artists = new JTextField(LibraryRepository.artistNamesForRelease(r));
        JComboBox<ReleaseType> type = new JComboBox<>(ReleaseType.values());
        if (r.getType() != null) type.setSelectedItem(r.getType());
        JTextField date    = new JTextField(r.getReleaseDate());
        JTextField upc     = new JTextField(r.getUpc());
        JTextField mbid    = new JTextField(r.getMbid());
        if (!FormDialog.show(getComponent(), "Edit Release",
                new String[]{"Title", "Artists (comma-separated)", "Type", "Date (YYYY-MM-DD)", "UPC", "MBID"},
                new JComponent[]{title, artists, type, date, upc, mbid})) return;
        if (title.getText().isBlank()) return;
        try {
            r.setName(title.getText().trim());
            r.setType((ReleaseType) type.getSelectedItem());
            r.setReleaseDate(blankToNull(date.getText()));
            r.setUpc(blankToNull(upc.getText()));
            r.setMbid(blankToNull(mbid.getText()));
            DatabaseManager.getInstance().dao(MusicRelease.class).update(r);
            LibraryRepository.setReleaseArtists(r, LibraryRepository.parseArtistNames(artists.getText()));
        } catch (Exception e) {
            showError(e);
        }
    }

    @Override
    protected void onDelete(int row) {
        MusicRelease r = rows.get(row);
        if (!confirmDelete("release \"" + r.getName() + "\"")) return;
        try {
            DatabaseManager.getInstance().dao(ReleaseCreatorCredit.class).delete(
                DatabaseManager.getInstance().dao(ReleaseCreatorCredit.class).queryForEq("release_id", r.getId()));
            DatabaseManager.getInstance().dao(MusicRelease.class).delete(r);
        } catch (Exception e) {
            showError(e);
        }
    }

    /**
     * A release drags as a single {@link PlayableKind#RELEASE} item (URI
     * {@code spacify:release:<id>}); its tracks form the {@link PlayableRef#expansion()}
     * that replaces it when the drop's expand (Alt) modifier is held.
     */
    @Override
    protected PlayableRef dragItemAt(int row) {
        MusicRelease r = rows.get(row);
        String artists = LibraryRepository.artistNamesForRelease(r);
        List<Playable> tracks = new ArrayList<>();
        long total = 0;
        try {
            List<Track> ts = DatabaseManager.getInstance().dao(Track.class).queryForEq("release_id", r.getId());
            ts.sort(LibraryRepository.ALBUM_ORDER);
            for (Track t : ts) { tracks.add(t); total += t.getDurationMs(); }
        } catch (Exception ignored) {}
        return new PlayableRef(PlayableKind.RELEASE, "spacify:release:" + r.getId(),
                r.getName(), artists, total, tracks);
    }

    private static String blankToNull(String s) {
        return (s == null || s.isBlank()) ? null : s.trim();
    }

    @Override public boolean acceptsUri(String uri) { return "spacify:library:releases".equals(uri); }
    @Override public String getName() { return "Releases"; }
}
