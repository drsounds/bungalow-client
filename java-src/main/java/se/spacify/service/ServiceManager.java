package se.spacify.service;

import se.spacify.aspect.BaseAspectManager;

import se.spacify.ui.MainWindow;

import java.util.*;

/**
 * Central singleton registry for Services and Features.
 * Call register() for each, then startAll() to drive onCreate → onStart.
 */
public class ServiceManager extends BaseAspectManager<Service> {

    // ── Service registration ──────────────────────────────────────────────────

    public ServiceManager(MainWindow mainWindow) {
        super(mainWindow);
        //TODO Auto-generated constructor stub
    }

    @Override
    public void register(Service service) {
        super.register(service);
        service.onCreate();
    }

    @Override
    public void unregister(String serviceId) {
        Service s = get(serviceId);
        super.unregister(serviceId);
        if (s != null) { s.onStop(); s.onDestroy(); }
    }

    /** Unregister a specific service instance (e.g. when a plugin is disabled). */
    public void unregister(Service service) {
        if (service != null) unregister(service.getId());
    }

    public void startAll() {
        for (Service s : getNodes().values()) s.onStart();
    }

    public void stopAll() {
        for (Service s : getNodes().values()) s.onStop();
    }

    public void shutdownAll() {
        for (Service s : getNodes().values()) { s.onStop(); s.onDestroy(); }
        getNodes().clear();
    }

    public Service get(String id) {
        return getNodes().get(id);
     }

    /**
     * Returns the first registered service exposing the given aspect, or null.
     * {@code aspect} may be {@link Service} itself, a concrete service class, or
     * any capability interface (e.g. {@link se.spacify.app.media.service.MediaService},
     * {@link AuthAspect}); only the services implementing it are considered.
     */
    @SuppressWarnings("unchecked")
    public <T> T getService(Class<T> aspect) {
        for (Service s : getNodes().values())
            if (aspect.isInstance(s)) return (T) s;
        return null;
    }

    /** Returns all registered services exposing the given aspect. */
    @SuppressWarnings("unchecked")
    public <T> List<T> getServices(Class<T> aspect) {
        List<T> result = new ArrayList<>();
        for (Service s : getNodes().values())
            if (aspect.isInstance(s)) result.add((T) s);
        return result;
    }

    // ── Feature registration ──────────────────────────────────────────────────

}
