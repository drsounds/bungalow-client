package se.spacify.app.music.model;

/**
 * The kind of content a playable reference points at — a single track/recording,
 * a whole release (album), or a whole playlist. Carried alongside a
 * {@link PlayableRef} when content is dragged into a playlist so the resulting
 * playlist item remembers what it is, rather than being flattened to "a track".
 *
 * <p>The persisted playlist row stores this explicitly, but when a row predates
 * the field (or a caller doesn't supply one) it is recovered from the content
 * URI's scheme via {@link #fromUri(String)} — the two agree by construction, so
 * either source is authoritative.
 */
public enum PlayableKind {

    TRACK,
    RELEASE,
    PLAYLIST;

    /** The stored, lower-case id (e.g. {@code "track"}); the inverse of {@link #fromId}. */
    public String id() {
        return name().toLowerCase(java.util.Locale.ROOT);
    }

    /** The kind for a stored id, or {@code null} if blank/unrecognised. */
    public static PlayableKind fromId(String id) {
        if (id == null || id.isBlank())
            return null;
        try {
            return valueOf(id.trim().toUpperCase(java.util.Locale.ROOT));
        } catch (IllegalArgumentException e) {
            return null;
        }
    }

    /**
     * Derive the kind from a content URI's scheme. For {@code musik:} URIs
     * (RFC-0002): {@code musik:upc:…}→{@link #RELEASE}, anything else
     * {@code musik:…}→{@link #TRACK}. For legacy {@code spacify:} content URIs:
     * {@code spacify:release:…}→{@link #RELEASE}, {@code spacify:playlist:…}→
     * {@link #PLAYLIST}. Defaults to {@link #TRACK} for anything else playable,
     * and {@code null} for a null URI.
     */
    public static PlayableKind fromUri(String uri) {
        if (uri == null)
            return null;
        if (uri.startsWith("musik:upc:"))        return RELEASE;
        if (uri.startsWith("musik:"))            return TRACK;
        if (uri.startsWith("spacify:release:"))  return RELEASE;
        if (uri.startsWith("spacify:playlist:")) return PLAYLIST;
        return TRACK;
    }
}
