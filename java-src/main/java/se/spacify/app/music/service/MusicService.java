package se.spacify.app.music.service;

import java.util.List;

import se.spacify.app.music.model.Recording;
import se.spacify.app.media.service.MediaService;

/**
 * Music-streaming aspect: ISRC- and title-based loading on top of the generic
 * {@link MediaService} playback aspect.
 */
public interface MusicService extends MediaService {

    /**
     * One concrete, playable candidate offered to the "Play with…" chooser. A
     * non-null {@code uri} plays exactly that target via {@link #loadUri}; else
     * {@code byIsrc} selects ISRC vs. title/artist loading.
     */
    record Candidate(String uri, String title, String artist, boolean byIsrc) {}

    /**
     * Concrete candidate matches for the request, shown in the chooser. The
     * default returns a single candidate from the existing lookups (so the chooser
     * keeps loading by ISRC/title); services that can offer several distinct
     * candidates — e.g. YouTube search results — should override and set a concrete
     * {@code uri} on each so the user's pick plays that exact item, not a re-search.
     */
    default List<Candidate> findCandidates(String isrc, String title, String artist) {
        if (isrc != null && !isrc.isBlank()) {
            Recording r = lookup(isrc);
            if (r != null) return List.of(new Candidate(null, r.getName(), title, true));
        }
        Recording r = lookupByTitleArtist(title, artist);
        if (r != null) return List.of(new Candidate(null, r.getName(), artist, false));
        return List.of();
    }

    /** Load and queue the recording identified by its ISRC code. */
    void loadByIsrc(String isrc);

    /** Best-effort lookup by title + artist name (fuzzy / exact depends on impl). */
    void loadByTitleArtist(String title, String artist);

    /**
     * Look up a Recording by ISRC without loading it for playback.
     * Returns null if not found.
     */
    Recording lookup(String isrc);

    /**
     * Best-effort lookup by title + artist without loading it for playback.
     * Returns null if not found. Default implementation finds nothing;
     * Services that can resolve metadata should override.
     */
    default Recording lookupByTitleArtist(String title, String artist) { return null; }
}
