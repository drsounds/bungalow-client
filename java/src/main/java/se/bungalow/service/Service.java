package se.bungalow.service;

import javax.swing.*;

import se.bungalow.aspect.Aspect;

/**
 * Core contract for every Bungalow Service: stable identity plus an
 * Android-style lifecycle. Capabilities a Service may additionally take on
 * (streaming, discovery, purchases, authentication, …) are modelled as separate
 * <em>aspect</em> interfaces ({@link se.bungalow.app.media.service.MediaService},
 * {@link se.bungalow.app.catalogue.service.MusicCatalogueService},
 * {@link AuthAspect}, …). Because these are interfaces, a single Service object
 * can implement any combination of them, and callers select Services by aspect
 * via {@link ServiceManager#getServices(Class)}.
 */
public interface Service extends Aspect {

    // ── Identity ──────────────────────────────────────────────────────────────

    String getId();

    String getName();

    /** Optional icon shown in Service-selection UI. */
    default ImageIcon getServiceIcon() { return null; }

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /** Called once when the Service is registered with ServiceManager. */
    default void onCreate() {}

    /** Called when ServiceManager.startAll() is invoked. */
    default void onStart() {}

    /** Called when ServiceManager.stopAll() is invoked. */
    default void onStop() {}

    /** Called once when the Service is permanently removed from ServiceManager. */
    default void onDestroy() {}
}
