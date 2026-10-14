package se.spacify.app.playlist;

import java.util.ArrayList;
import java.util.List;

/**
 * Change-notification bus for playlists (mirrors
 * {@link se.spacify.library.LibraryEvents} and
 * {@link se.spacify.web.BookmarkEvents}). The playlist service fires
 * {@link #fireChanged()} after any mutation; views that mirror playlists
 * (the sidebar subtree, the tracks view's playlist grouping) subscribe via
 * {@link #addListener(Runnable)}.
 */
public final class PlaylistEvents {

    private static final List<Runnable> listeners = new ArrayList<>();

    private PlaylistEvents() {}

    public static void addListener(Runnable r) { listeners.add(r); }

    public static void removeListener(Runnable r) { listeners.remove(r); }

    public static void fireChanged() {
        for (Runnable r : List.copyOf(listeners)) r.run();
    }
}
