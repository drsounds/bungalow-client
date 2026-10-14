package se.spacify.app.media.service;

import se.spacify.service.Service;
import se.spacify.service.media.PlaybackSupport;

/**
 * Streaming / playback aspect. A {@link Service} that can play media implements
 * this interface; the concrete class typically composes a {@link PlaybackSupport}
 * to manage its listener list and fire events. Decoupling playback into an
 * aspect lets one Service combine streaming with other aspects (discovery,
 * purchases, …).
 */
public interface MediaService extends Service {

    // ── State enum ────────────────────────────────────────────────────────────

    enum PlaybackState { IDLE, LOADING, PLAYING, PAUSED, STOPPED, ERROR }

    // ── Listener interface ────────────────────────────────────────────────────

    interface PlaybackListener {
        default void onStateChanged(PlaybackState state) {}
        default void onPositionChanged(long positionMs, long durationMs) {}
        default void onTrackChanged(String title, String artist, String album) {}
        default void onError(Exception e) {}
        /** Fired when the current track reaches its natural end. */
        default void onCompleted() {}
    }

    // ── Listener registration ─────────────────────────────────────────────────

    void addPlaybackListener(PlaybackListener l);
    void removePlaybackListener(PlaybackListener l);

    // ── Playback API ──────────────────────────────────────────────────────────

    void play();
    void pause();
    void stop();
    void seek(long positionMs);
    void loadUri(String uri);

    PlaybackState getPlaybackState();
    long getPositionMs();
    long getDurationMs();

    // ── Optional visual surface ─────────────────────────────────────────────────

    /**
     * The playback surface this Service contributes to the Now Playing panel, or
     * {@code null} if it needs none (e.g. audio-only local playback). Services
     * that integrate an embedded player (YouTube via JCEF, …) return their
     * component here; the panel shows the active Service's component.
     */
    default MediaServicePlayerComponent getPlayerComponent() { return null; }
}
