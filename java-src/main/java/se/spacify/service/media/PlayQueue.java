package se.spacify.service.media;

import java.util.ArrayList;
import java.util.List;

/**
 * Application-wide play context. When a track is activated in a view, that
 * view's visible rows become the queue and playback starts at the activated
 * position. The right-hand queue panel renders {@link #getItems()} and
 * highlights {@link #getCurrentIndex()}; the player-bar skip controls drive
 * {@link #next()} / {@link #previous()}.
 */
public final class PlayQueue {

    private static final PlayQueue INSTANCE = new PlayQueue();
    public static PlayQueue getInstance() { return INSTANCE; }

    private PlayQueue() {}

    private final List<PlayQueueItem> items   = new ArrayList<>();
    private int                       current = -1;
    private final List<Runnable>      listeners = new ArrayList<>();

    /** Subscribe to queue / current-index changes (fired on the calling thread). */
    public void addChangeListener(Runnable r) { listeners.add(r); }

    /** Immutable snapshot of the current queue contents. */
    public List<PlayQueueItem> getItems() { return List.copyOf(items); }

    /** Index of the entry currently playing, or -1 when the queue is empty. */
    public int getCurrentIndex() { return current; }

    /** Play-identity of the entry currently playing, or null. */
    public String getCurrentKey() {
        return (current >= 0 && current < items.size()) ? items.get(current).getKey() : null;
    }

    /** Whether {@code key} identifies the entry currently playing. */
    public boolean isCurrentKey(String key) {
        return key != null && key.equals(getCurrentKey());
    }

    /**
     * Replace the queue with {@code newItems} and start playing at
     * {@code startIndex}. This is the entry point used when a view "becomes" the
     * play context on double-click.
     */
    public void setQueueAndPlay(List<PlayQueueItem> newItems, int startIndex) {
        items.clear();
        items.addAll(newItems);
        current = -1;
        playAt(startIndex);
    }

    /** Play the entry at {@code index}; no-op if out of range. */
    public void playAt(int index) {
        if (index < 0 || index >= items.size()) return;
        current = index;
        items.get(index).play();
        notifyListeners();
    }

    /** Advance to the next entry, if any. */
    public void next() {
        if (current + 1 < items.size()) playAt(current + 1);
    }

    /** Return to the previous entry, if any. */
    public void previous() {
        if (current - 1 >= 0) playAt(current - 1);
    }

    private void notifyListeners() {
        for (Runnable r : List.copyOf(listeners)) r.run();
    }
}
