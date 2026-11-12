package se.bungalow.app.playlist;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;

import se.bungalow.app.Application;
import se.bungalow.app.ApplicationContext;

import se.bungalow.app.playlist.concept.PlaylistConcept;
import se.bungalow.app.playlist.service.LocalPlaylistService;

/**
 * Built-in plugin providing playlists: the database-backed
 * {@link LocalPlaylistService} (the editable local store) plus the
 * {@link PlaylistConcept} that contributes the "Playlists" sidebar subtree and
 * the {@code bungalow:playlist:<uuid>} view.
 */
public class PlaylistApplication extends Application {

    @Override
    public String getId() {
        return "playlist";
    }

    @Override
    public String getName() {
        return "Playlists";
    }

    @Override
    public void onRegister(AspectManager<? extends Aspect> aspectManager) {
        // No aspect-manager wiring needed.
    }

    @Override
    public void onActivate(ApplicationContext ctx) {
        ctx.registerService(new LocalPlaylistService());
        ctx.registerConcept(new PlaylistConcept(this));
    }
}
