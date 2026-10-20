package se.bungalow.app.music.model;

/**
 * Anything that can be queued for playback.
 * Both Recording and Track implement this interface.
 */
public interface Playable {
    /** bungalow: URI handed to a MusicService for loading. */
    String getPlayUri();

    long getDurationMs();

    String getName();
}
