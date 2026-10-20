package se.bungalow.app.library;

import java.util.ArrayList;
import java.util.List;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;
import se.bungalow.db.DatabaseManager;
import se.bungalow.db.LibraryRepository;
import se.bungalow.app.music.model.Artist;
import se.bungalow.app.music.model.Recording;
import se.bungalow.app.music.model.MusicRelease;
import se.bungalow.app.music.model.Track;
import se.bungalow.search.EntityKind;
import se.bungalow.search.SearchProvider;
import se.bungalow.search.SearchResult;

/**
 * Searches the local library (SQLite) for matching artists, releases and songs.
 * Results navigate to the corresponding library views.
 */
public final class LibrarySearchProvider implements SearchProvider {

    private static final long LIMIT = 8;

    @Override public String getId()   { return "search.library"; }
    @Override public String getName() { return "Library"; }
    @Override public void onRegister(AspectManager<? extends Aspect> manager) {}

    @Override
    public List<SearchResult> search(String query) {
        List<SearchResult> out = new ArrayList<>();
        String like = "%" + query + "%";
        DatabaseManager db = DatabaseManager.getInstance();
        try {
            for (Artist a : db.dao(Artist.class).queryBuilder().limit(LIMIT).where().like("name", like).query()) {
                out.add(new SearchResult(EntityKind.ARTIST, a.getName(), "",
                    "bungalow:library:artist:" + a.getId(), getName()));
            }
            for (MusicRelease r : db.dao(MusicRelease.class).queryBuilder().limit(LIMIT).where().like("title", like).query()) {
                out.add(new SearchResult(EntityKind.RELEASE, r.getName(),
                    LibraryRepository.artistNamesForRelease(r),
                    "bungalow:library:release:" + r.getId(), getName()));
            }
            for (Recording rec : db.dao(Recording.class).queryBuilder().limit(LIMIT).where().like("title", like).query()) {
                out.add(new SearchResult(EntityKind.RECORDING, rec.getName(),
                    LibraryRepository.artistNamesForRecording(rec), releaseUri(db, rec), getName()));
            }
        } catch (Exception ignored) {
            // DB error → whatever we collected so far.
        }
        return out;
    }

    /** Navigate a song to the album it appears on, when known. */
    private static String releaseUri(DatabaseManager db, Recording rec) {
        try {
            List<Track> tracks = db.dao(Track.class).queryForEq("recording_id", rec.getId());
            if (!tracks.isEmpty() && tracks.get(0).getRelease() != null) {
                return "bungalow:library:release:" + tracks.get(0).getRelease().getId();
            }
        } catch (Exception ignored) {}
        return null;
    }
}
