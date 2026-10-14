package se.spacify.app.music.model;

/**
 * Anything that can be queued for playback.
 * Both Recording and Track implement this interface.
 */
public interface Playable {
    /** spacify: URI handed to a MusicService for loading. */
    String getPlayUri();

    long getDurationMs();

    String getName();
}
