package se.spacify.service.media;

import java.util.ArrayList;
import java.util.List;

import se.spacify.app.media.service.MediaService;
import se.spacify.app.media.service.MediaService.PlaybackListener;
import se.spacify.app.media.service.MediaService.PlaybackState;

/**
 * Reusable listener bookkeeping for {@link MediaService} implementations.
 * Holds the {@link PlaybackListener} list and fires events to all subscribers —
 * the stateful part that the {@code MediaService} interface itself can't carry.
 * Concrete Services compose one of these and delegate
 * {@code addPlaybackListener} / {@code removePlaybackListener} to it.
 */
public final class PlaybackSupport {

    private final List<PlaybackListener> listeners = new ArrayList<>();

    public void addPlaybackListener(PlaybackListener l)    { listeners.add(l); }
    public void removePlaybackListener(PlaybackListener l) { listeners.remove(l); }

    public void fireStateChanged(PlaybackState state) {
        for (PlaybackListener l : List.copyOf(listeners)) l.onStateChanged(state);
    }

    public void firePositionChanged(long posMs, long durMs) {
        for (PlaybackListener l : List.copyOf(listeners)) l.onPositionChanged(posMs, durMs);
    }

    public void fireTrackChanged(String title, String artist, String album) {
        for (PlaybackListener l : List.copyOf(listeners)) l.onTrackChanged(title, artist, album);
    }

    public void fireError(Exception e) {
        for (PlaybackListener l : List.copyOf(listeners)) l.onError(e);
    }

    public void fireCompleted() {
        for (PlaybackListener l : List.copyOf(listeners)) l.onCompleted();
    }
}
