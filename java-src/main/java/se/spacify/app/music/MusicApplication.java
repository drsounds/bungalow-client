package se.spacify.app.music;

import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;

import se.spacify.library.LibraryEvents;

import se.spacify.app.Application;
import se.spacify.app.ApplicationContext;

import se.spacify.app.music.concept.MusicConcept;

/**
 * Built-in plugin providing the music library: the data-backed views (tracks,
 * recordings, releases, artists, local files, detail pages, search, playlists)
 * registered into the {@code spacify:library*} URI space, plus the "Your Library"
 * sidebar subtree with live Releases/Artists lists kept in sync via
 * {@link LibraryEvents}.
 */
public class MusicApplication extends Application {


    @Override
    public String getId() {
        // TODO Auto-generated method stub
        return "music";
    }

    @Override
    public String getName() {
        // TODO Auto-generated method stub
        return "Music";
    }

    @Override
    public void onRegister(AspectManager<? extends Aspect> aspectManager) {
        // TODO Auto-generated method stub

    }
    @Override
    public void onActivate(ApplicationContext ctx) {
        ctx.registerConcept(new MusicConcept(this));
    }
}
