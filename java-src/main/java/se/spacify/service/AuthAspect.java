package se.spacify.service;

import java.awt.Component;
import java.util.function.Consumer;

/**
 * Authentication aspect. Implemented by Services that require a logged-in
 * account (streaming back-ends, purchase stores, …). Services that need no
 * sign-in — e.g. local file playback — simply don't implement this interface,
 * and UI can discover the ones that do via
 * {@link ServiceManager#getServices(Class)}.
 */
public interface AuthAspect {

    boolean isAuthenticated();

    /**
     * Present whatever authentication UI the Service requires.
     * Called on the EDT; the Service is responsible for any background work.
     */
    void login(Component parent, Runnable onSuccess, Consumer<Exception> onError);

    void logout();

    /**
     * Returns the currently logged-in account object, or null.
     * Implementations should narrow the return type.
     */
    Object getAccount();
}
