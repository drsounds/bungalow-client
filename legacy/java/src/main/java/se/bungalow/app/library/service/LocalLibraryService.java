package se.bungalow.app.library.service;

import com.j256.ormlite.dao.Dao;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;
import se.bungalow.db.DatabaseManager;
import se.bungalow.app.localmusic.model.LocalFile;
import se.bungalow.app.music.model.Artist;
import se.bungalow.app.music.model.MusicRelease;
import se.bungalow.app.music.model.Recording;
import se.bungalow.app.music.model.Track;

import java.sql.SQLException;
import java.util.Comparator;
import java.util.List;
import java.util.function.Function;

/**
 * The {@link LibraryService} for the on-disk library ({@code ~/.bungalow/library.db}):
 * every artist, release, recording and track the user has added or scanned,
 * sorted by name. Registered by the library concept so other apps can browse
 * the local collection through {@code ServiceManager.getServices(LibraryService.class)}
 * like any remote library.
 */
public class LocalLibraryService implements LibraryService {

    @Override public String getId()   { return "bungalow.local.library"; }
    @Override public String getName() { return "Local Library"; }
    @Override public void onRegister(AspectManager<? extends Aspect> manager) {}

    @Override public List<Artist>       getArtists()    { return sortedByName(Artist.class, Artist::getName); }
    @Override public List<MusicRelease> getReleases()   { return sortedByName(MusicRelease.class, MusicRelease::getName); }
    @Override public List<Recording>    getRecordings() { return sortedByName(Recording.class, Recording::getName); }
    @Override public List<Track>        getTracks()     { return sortedByName(Track.class, Track::getName); }
    @Override public List<LocalFile>    getLocalFiles() { return sortedByName(LocalFile.class, LocalFile::getName); }

    /** Recordings whose name or ISRC contains {@code query} (case-insensitive). */
    @Override
    public List<Recording> search(String query) {
        if (query == null || query.isBlank()) return List.of();
        String like = "%" + query.trim() + "%";
        try {
            Dao<Recording, Integer> dao = DatabaseManager.getInstance().dao(Recording.class);
            return dao.queryBuilder().orderBy("name", true)
                      .where().like("name", like).or().like("isrc", like)
                      .query();
        } catch (SQLException e) {
            return List.of();
        }
    }

    private static <T> List<T> sortedByName(Class<T> entity, Function<T, String> name) {
        try {
            List<T> all = DatabaseManager.getInstance().dao(entity).queryForAll();
            all.sort(Comparator.comparing(name, Comparator.nullsLast(String.CASE_INSENSITIVE_ORDER)));
            return all;
        } catch (SQLException e) {
            return List.of();
        }
    }
}
