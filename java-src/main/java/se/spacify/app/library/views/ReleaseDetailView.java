package se.spacify.app.library.views;
import se.spacify.app.music.controls.MusicTable;
import se.spacify.app.music.views.AbstractMusicListView;

import se.spacify.db.DatabaseManager;
import se.spacify.db.LibraryRepository;
import se.spacify.app.music.model.Recording;
import se.spacify.app.music.model.MusicRelease;
import se.spacify.app.music.model.Track;
import se.spacify.navigation.ViewStack;
import se.spacify.service.media.PlayRequest;

import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Read-only detail page for a single release, reached from the sidebar via
 * {@code spacify:library:release:<id>}. Lists the album's tracks; double-click
 * resolves and plays a track across the registered music Services.
 */
public class ReleaseDetailView extends AbstractMusicListView {
    public ReleaseDetailView(ViewStack viewStack) {
        super(viewStack);
        //TODO Auto-generated constructor stub
    }
    private static final Pattern URI = Pattern.compile("spacify:library:release:(\\d+)");

    private final List<Track> rows = new ArrayList<>();
    private int releaseId = -1;

    @Override protected boolean isEditable() { return false; }

    @Override protected List<MusicTable.Column> getColumns() {
        return List.of(
            column("number",    "#"),
            column("recording", "Recording"),
            column("artists",   "Artists"));
    }

    @Override
    public void navigate(String uri) {
        Matcher m = URI.matcher(uri);
        releaseId = m.matches() ? Integer.parseInt(m.group(1)) : -1;
    }

    @Override
    protected void reload() {
        rows.clear();
        musicTable.clear();
        if (releaseId < 0) { setHeader(null); return; }
        try {
            MusicRelease release = DatabaseManager.getInstance().dao(MusicRelease.class).queryForId(releaseId);
            if (release == null) { setHeader("Release not found"); return; }
            String artists = LibraryRepository.artistNamesForRelease(release);
            setHeader(artists.isBlank() ? release.getName() : release.getName() + " — " + artists);

            List<Track> tracks = DatabaseManager.getInstance().dao(Track.class).queryForEq("release_id", releaseId);
            tracks.sort(LibraryRepository.ALBUM_ORDER);
            for (Track t : tracks) {
                rows.add(t);
                Recording rec = t.getRecording();
                addRow(row()
                    .set("number",    t.getTrackNumber())
                    .set("recording", rec != null ? rec.getName() : "")
                    .set("artists",   rec != null ? LibraryRepository.artistNamesForRecording(rec) : ""));
            }
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

    @Override public boolean acceptsUri(String uri) { return uri != null && URI.matcher(uri).matches(); }
    @Override public String getName() { return "Release"; }
}
