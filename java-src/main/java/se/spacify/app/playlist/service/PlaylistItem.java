package se.spacify.app.playlist.service;

import se.spacify.app.music.model.Playable;

/**
 * A lightweight, display-ready {@link Playable} reconstructed from a persisted
 * playlist row. Carries the denormalised metadata stored on the row (title,
 * artist, duration) plus the {@code spacify:} play URI, so a playlist renders and
 * plays without any cross-service lookup. The {@link #getArtist() artist}
 * accessor is extra to the {@link Playable} contract — views may read it when an
 * item is a {@code PlaylistItem}.
 */
public final class PlaylistItem implements Playable {

    private final String uri;
    private final String title;
    private final String artist;
    private final long   durationMs;

    public PlaylistItem(String uri, String title, String artist, long durationMs) {
        this.uri        = uri;
        this.title      = title;
        this.artist     = artist;
        this.durationMs = durationMs;
    }

    @Override public String getPlayUri()    { return uri; }
    @Override public String getName()      { return title; }
    @Override public long   getDurationMs() { return durationMs; }

    public String getArtist() { return artist; }
}
