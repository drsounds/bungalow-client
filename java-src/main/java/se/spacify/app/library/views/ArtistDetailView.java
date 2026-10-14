package se.spacify.app.library.views;
import se.spacify.app.music.controls.MusicTable;
import se.spacify.app.music.views.AbstractMusicListView;

import se.spacify.db.DatabaseManager;
import se.spacify.db.LibraryRepository;
import se.spacify.app.music.model.Artist;
import se.spacify.app.music.model.Recording;
import se.spacify.app.music.model.RecordingCreatorCredit;
import se.spacify.navigation.ViewStack;
import se.spacify.service.media.PlaybackCoordinator;

import se.spacify.service.media.PlayQueueItem;

import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Read-only detail page for a single artist, reached from the sidebar via
 * {@code spacify:library:artist:<id>}. Lists the artist's recordings and the
 * album each appears on; double-click resolves and plays across Services.
 */
public class ArtistDetailView extends AbstractMusicListView {

    public ArtistDetailView(ViewStack viewStack) {
        super(viewStack);
    }
    private static final Pattern URI = Pattern.compile("spacify:library:artist:(\\d+)");

    private final List<Recording> rows = new ArrayList<>();
    private int artistId = -1;

    @Override protected boolean isEditable() { return false; }

    @Override protected List<MusicTable.Column> getColumns() {
        return List.of(
            column("recording", "Recording"),
            column("album",     "Album"));
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

            List<RecordingCreatorCredit> credits = DatabaseManager.getInstance()
                .dao(RecordingCreatorCredit.class).queryForEq("artist_id", artistId);
            for (RecordingCreatorCredit c : credits) {
                Recording rec = c.getRecording();
                if (rec == null) continue;
                rows.add(rec);
                addRow(row()
                    .set("recording", rec.getName())
                    .set("album",     LibraryRepository.albumForRecording(rec)));
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

    @Override public boolean acceptsUri(String uri) { return uri != null && URI.matcher(uri).matches(); }
    @Override public String getName() { return "Artist"; }
}
