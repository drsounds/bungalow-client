package se.spacify.app.music.model;

import java.util.List;

/**
 * A kind-tagged, display-ready reference to something playable, used as the
 * payload when a list row is dragged into a playlist. It is a {@link Playable}
 * (so it hands straight to {@code PlaylistService.addToPlaylist}) that also
 * carries its {@link PlayableKind} and, for aggregate content (a release, a
 * playlist), an {@link #expansion() expansion} — the child playables to add
 * instead when the user holds the expand modifier while dropping.
 *
 * <p>For a plain track the kind is {@link PlayableKind#TRACK} and the expansion
 * is empty; dropping it always adds exactly the one item.
 */
public final class PlayableRef implements Playable {

    private final PlayableKind kind;
    private final String uri;
    private final String title;
    private final String artist;
    private final long   durationMs;
    private final List<Playable> expansion;

    public PlayableRef(PlayableKind kind, String uri, String title, String artist,
                       long durationMs, List<Playable> expansion) {
        this.kind       = kind;
        this.uri        = uri;
        this.title      = title;
        this.artist     = artist;
        this.durationMs = durationMs;
        this.expansion  = expansion == null ? List.of() : List.copyOf(expansion);
    }

    /** A single, non-expandable track reference. */
    public static PlayableRef track(String uri, String title, String artist, long durationMs) {
        return new PlayableRef(PlayableKind.TRACK, uri, title, artist, durationMs, List.of());
    }

    @Override public String getPlayUri()    { return uri; }
    @Override public String getName()       { return title; }
    @Override public long   getDurationMs() { return durationMs; }

    public String       getArtist()  { return artist; }
    public PlayableKind getKind()    { return kind; }

    /** The child playables to add when expanded; empty when this is a single item. */
    public List<Playable> expansion() { return expansion; }

    /** Whether holding the expand modifier would add more than this one item. */
    public boolean isExpandable() { return !expansion.isEmpty(); }
}
