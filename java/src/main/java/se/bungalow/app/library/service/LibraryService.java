package se.bungalow.app.library.service;

import se.bungalow.app.music.model.Artist;
import se.bungalow.app.localmusic.model.LocalFile;
import se.bungalow.app.music.model.Recording;
import se.bungalow.app.music.model.MusicRelease;
import se.bungalow.app.music.model.Track;
import se.bungalow.service.Service;

import java.util.List;

/**
 * Library aspect: browsing a collection of music a user owns or has saved.
 * Implemented by Services that expose such a collection — the local catalogue,
 * or a remote account's saved library — and discovered via
 * {@link se.bungalow.service.ServiceManager#getServices(Class)}. A Service may
 * combine this with the streaming aspect ({@link se.bungalow.app.media.service.MediaService})
 * to both browse and play, or with the discovery aspect
 * ({@link se.bungalow.app.catalogue.service.MusicCatalogueService}).
 */
public interface LibraryService extends Service {

    List<Artist>    getArtists();
    List<MusicRelease>   getReleases();
    List<Recording> getRecordings();
    List<Track>     getTracks();

    /** Locally-backed files, if this library has any; default none. */
    default List<LocalFile> getLocalFiles() { return List.of(); }

    /** Free-text search across the collection; default finds nothing. */
    default List<Recording> search(String query) { return List.of(); }
}
