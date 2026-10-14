package se.spacify.library;

import java.util.ArrayList;
import java.util.List;

/**
 * Minimal change-notification bus for the music library. UI that mutates the
 * catalogue (scans, CRUD) calls {@link #fireChanged()}; views that mirror it
 * (e.g. the sidebar tree) subscribe via {@link #addListener(Runnable)}.
 */
public final class LibraryEvents {

    private static final List<Runnable> listeners = new ArrayList<>();

    private LibraryEvents() {}

    public static void addListener(Runnable r) { listeners.add(r); }

    public static void removeListener(Runnable r) { listeners.remove(r); }

    public static void fireChanged() {
        for (Runnable r : List.copyOf(listeners)) r.run();
    }
}
