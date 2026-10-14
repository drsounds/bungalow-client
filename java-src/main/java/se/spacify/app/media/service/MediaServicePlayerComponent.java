package se.spacify.app.media.service;

import se.spacify.controls.Panel;
import se.spacify.service.media.PlaybackCoordinator;

/**
 * The per-Service playback surface shown at the bottom of the right-hand
 * Now Playing panel. A {@link MediaService} that needs a visual surface (e.g. an
 * embedded web player) returns one from {@link MediaService#getPlayerComponent()};
 * audio-only Services (local files) return {@code null} and contribute no surface.
 *
 * <p>The host shows exactly one component at a time — the one belonging to the
 * Service that resolved the current playable (see
 * {@link PlaybackCoordinator#getActiveService()}). {@link #onActivated()} is
 * called when this component becomes that surface and {@link #onDeactivated()}
 * when another Service takes over, so a component can start/suspend heavy
 * resources (such as a CEF browser) accordingly.
 */
public abstract class MediaServicePlayerComponent extends Panel {

    protected MediaServicePlayerComponent() {
        getComponent().setOpaque(false);
    }

    /** Called when this component becomes the active (visible) playback surface. */
    public void onActivated() {}

    /** Called when another Service's component replaces this one. */
    public void onDeactivated() {}
}
