package se.spacify.skinning;

import java.util.*;

import se.spacify.aspect.BaseAspectManager;
import se.spacify.ui.MainWindow;

/**
 * Central singleton registry for Skins and Features.
 * Call register() for each, then startAll() to drive onCreate → onStart.
 */
public class SkinManager extends BaseAspectManager<Skin> {
    public SkinManager(MainWindow mainWindow) {
        super(mainWindow);
    }
    // ── Skin registration ──────────────────────────────────────────────────

    /**
     * Returns the first registered Skin exposing the given aspect, or null.
     * {@code aspect} may be {@link Skin} itself, a concrete Skin class, or
     * any capability interface (e.g. {@link se.spacify.Skin.media.MediaSkin},
     * {@link AuthAspect}); only the Skins implementing it are considered.
     */
    @SuppressWarnings("unchecked")
    public <T> T getSkin(Class<T> aspect) {
        for (Skin s : getNodes().values())
            if (aspect.isInstance(s)) return (T) s;
        return null;
    }

    /** Returns all registered Skins exposing the given aspect. */
    @SuppressWarnings("unchecked")
    public <T> List<T> getSkins(Class<T> aspect) {
        List<T> result = new ArrayList<>();
        for (Skin s : getNodes().values())
            if (aspect.isInstance(s)) result.add((T) s);
        return result;
    }
}
