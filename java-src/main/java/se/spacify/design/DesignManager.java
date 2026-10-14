package se.spacify.design;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;

import java.util.List;

import se.spacify.aspect.BaseAspectManager;

import se.spacify.service.AuthAspect;
import se.spacify.ui.MainWindow;

public class DesignManager extends BaseAspectManager<Design> {
    
    public DesignManager(MainWindow mainWindow) {
        super(mainWindow);
    }

    /**
     * Returns the first registered design exposing the given aspect, or null.
     * {@code aspect} may be {@link Design} itself, a concrete design class, or
     * any capability interface (e.g. {@link se.spacify.design.media.MediaDesign},
     * {@link AuthAspect}); only the designs implementing it are considered.
     */
    @SuppressWarnings("unchecked")
    public <T> T getDesign(Class<T> aspect) {
        for (Design s : getNodes().values())
            if (aspect.isInstance(s)) return (T) s;
        return null;
    }

    /** Returns all registered designs exposing the given aspect. */
    @SuppressWarnings("unchecked")
    public <T> List<T> getDesigns(Class<T> aspect) {
        List<T> result = new ArrayList<>();
        for (Design s : getNodes().values())
            if (aspect.isInstance(s)) result.add((T) s);
        return result;
    }

    public Collection<Design> all() { return Collections.unmodifiableCollection(getNodes().values()); }


    public Design get(String id) {
        return getNodes().get(id);
    }

}
