package se.spacify.service.media;

import se.spacify.app.music.model.Track;

/**
 * The full context of a request to play one track, carried from the originating
 * view down to {@link PlaybackCoordinator#resolveAndPlay}. It bundles the
 * metadata used to resolve the track across Services (ISRC, title, artist, and a
 * direct-URI fallback) together with the identity used to persist the user's
 * "Play with…" choice: the local {@link Track} (for a foreign-key binding) when
 * the play came from the library, plus a stable {@link #trackKey()} that also
 * works for non-library plays.
 *
 * <p>Display fields ({@link #title()}, {@link #artist()}, {@link #durationMs()})
 * double as the play-queue row metadata, and {@link #key()} as the queue's
 * now-playing identity.
 */
public final class PlayRequest {

    private final Track  track;       // nullable — present for library plays
    private final String isrc;
    private final String title;
    private final String artist;
    private final String fallbackUri; // direct play URI when no Service resolves it
    private final long   durationMs;

    public PlayRequest(Track track, String isrc, String title, String artist,
                       String fallbackUri, long durationMs) {
        this.track       = track;
        this.isrc        = blankToNull(isrc);
        this.title       = title  != null ? title  : "";
        this.artist      = artist != null ? artist : "";
        this.fallbackUri = blankToNull(fallbackUri);
        this.durationMs  = durationMs;
    }

    public Track   track()       { return track; }
    public String  isrc()        { return isrc; }
    public String  title()       { return title; }
    public String  artist()      { return artist; }
    public String  fallbackUri() { return fallbackUri; }
    public long    durationMs()  { return durationMs; }

    /**
     * Stable identity used to persist/look up the saved Service choice when there
     * is no {@link #track()}: the ISRC, else the play URI, else a title|artist key.
     */
    public String trackKey() {
        if (isrc        != null) return "isrc:" + isrc;
        if (fallbackUri != null) return fallbackUri;
        return "ta:" + title + "|" + artist;
    }

    /** Now-playing identity for the play queue (the play URI when available). */
    public String key() {
        return fallbackUri != null ? fallbackUri : trackKey();
    }

    private static String blankToNull(String s) {
        return (s == null || s.isBlank()) ? null : s;
    }
}
