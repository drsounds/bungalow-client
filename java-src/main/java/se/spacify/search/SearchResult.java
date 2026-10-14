package se.spacify.search;

/**
 * One hit returned by a {@link SearchProvider}. Typed by its {@link EntityKind}
 * (which section it appears under) and carrying a {@code uri} that is activated
 * on click — navigated to a view, or played if it's a playback URI
 * (e.g. {@code spacify:youtube:<id>}). {@code source} names the contributing
 * service (Library / MusicBrainz / YouTube …) for a badge.
 */
public record SearchResult(EntityKind kind, String title, String subtitle, String uri, String source) {

    public SearchResult(EntityKind kind, String title, String subtitle, String uri, String source) {
        this.kind = kind;
        this.title = title == null ? "" : title;
        this.subtitle = subtitle == null ? "" : subtitle;
        this.uri = uri;
        this.source = source == null ? "" : source;
    }
}
