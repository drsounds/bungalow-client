package se.spacify.ui.chrome;

import java.util.*;

import se.spacify.aspect.BaseAspectManager;
import se.spacify.ui.MainWindow;

/**
 * Central singleton registry for Services and Features.
 * Call register() for each, then startAll() to drive onCreate → onStart.
 */
public class ChromeManager extends BaseAspectManager<Chrome> {

    public ChromeManager(MainWindow mainWindow) {
        super(mainWindow);
    }

    // ── Service registration ──────────────────────────────────────────────────

    /**
     * Returns the first registered service exposing the given aspect, or null.
     * {@code aspect} may be {@link Service} itself, a concrete service class, or
     * any capability interface (e.g. {@link se.spacify.app.media.service.MediaService},
     * {@link AuthAspect}); only the services implementing it are considered.
     */
    @SuppressWarnings("unchecked")
    public <T> T getChrome(Class<T> aspect) {
        for (Chrome s : getNodes().values())
            if (aspect.isInstance(s)) return (T) s;
        return null;
    }

    /** Returns all registered services exposing the given aspect. */
    @SuppressWarnings("unchecked")
    public <T> List<T> getChromes(Class<T> aspect) {
        List<T> result = new ArrayList<>();
        for (Chrome s : getNodes().values())
            if (aspect.isInstance(s)) result.add((T) s);
        return result;
    }
}
