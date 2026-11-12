package se.bungalow.app.media;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;

import se.bungalow.library.LibraryEvents;

import se.bungalow.app.Application;
import se.bungalow.app.ApplicationContext;

import se.bungalow.app.media.concept.MediaConcept;

/**
 * Built-in plugin providing the music library: the data-backed views (tracks,
 * recordings, releases, artists, local files, detail pages, search, playlists)
 * registered into the {@code bungalow:library*} URI space, plus the "Your Library"
 * sidebar subtree with live Releases/Artists lists kept in sync via
 * {@link LibraryEvents}.
 */
public class MediaApplication extends Application {


    @Override
    public String getId() {
        // TODO Auto-generated method stub
        return "library";
    }

    @Override
    public String getName() {
        // TODO Auto-generated method stub
        return "Library";
    }

    @Override
    public void onRegister(AspectManager<? extends Aspect> aspectManager) {
        // TODO Auto-generated method stub

    }
    @Override
    public void onActivate(ApplicationContext ctx) {
        ctx.registerConcept(new MediaConcept(this));
    }
}
