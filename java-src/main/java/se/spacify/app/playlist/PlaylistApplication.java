package se.spacify.app.playlist;

import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;

import se.spacify.app.Application;
import se.spacify.app.ApplicationContext;

import se.spacify.app.playlist.concept.PlaylistConcept;
import se.spacify.app.playlist.service.LocalPlaylistService;

/**
 * Built-in plugin providing playlists: the database-backed
 * {@link LocalPlaylistService} (the editable local store) plus the
 * {@link PlaylistConcept} that contributes the "Playlists" sidebar subtree and
 * the {@code spacify:playlist:<uuid>} view.
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
