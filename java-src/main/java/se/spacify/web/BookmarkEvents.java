package se.spacify.web;

import java.util.ArrayList;
import java.util.List;

/**
 * Change-notification bus for bookmarks (mirrors LibraryEvents). The web view
 * fires {@link #fireChanged()} after a toggle; the sidebar's "Sites" subtree
 * subscribes via {@link #addListener(Runnable)}.
 */
public final class BookmarkEvents {

    private static final List<Runnable> listeners = new ArrayList<>();

    private BookmarkEvents() {}

    public static void addListener(Runnable r) { listeners.add(r); }

    public static void removeListener(Runnable r) { listeners.remove(r); }

    public static void fireChanged() {
        for (Runnable r : List.copyOf(listeners)) r.run();
    }
}
