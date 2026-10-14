package se.spacify.app.catalogue.service;

import se.spacify.app.music.model.Artist;
import se.spacify.app.music.model.Recording;
import se.spacify.app.music.model.MusicRelease;
import se.spacify.service.Service;

import java.util.List;

/**
 * Discovery aspect: catalogue browsing and search. Implementations can connect
 * to Discogs, MusicBrainz, etc. A Service may combine this aspect with the
 * streaming aspect ({@link se.spacify.app.media.service.MediaService}) to both find
 * and play music.
 */
public interface MusicCatalogueService extends Service {

    List<MusicRelease>   searchReleases(String query);
    List<Recording> searchRecordings(String query);
    List<Artist>    searchArtists(String query);

    MusicRelease   getReleaseByMbid(String mbid);
    Recording getRecordingByIsrc(String isrc);
    Artist    getArtistByIsni(String isni);

    // ── Browsing (contextual; drives the catalogue tree's drill-down) ───────────
    //
    // Most catalogues (MusicBrainz included) have no global "list everything"
    // feed — browse requests hang off a parent entity. These let the catalogue
    // views drill Artist → Releases → Recordings. Default to empty so catalogues
    // that don't support browsing simply show nothing rather than break.

    /** Releases credited to the given artist (by catalogue id, e.g. MBID). */
    default List<MusicRelease> browseReleasesByArtist(String artistId, int offset, int limit) {
        return List.of();
    }

    /** Recordings (tracks) appearing on the given release (by catalogue id). */
    default List<Recording> browseRecordingsByRelease(String releaseId, int offset, int limit) {
        return List.of();
    }
}
