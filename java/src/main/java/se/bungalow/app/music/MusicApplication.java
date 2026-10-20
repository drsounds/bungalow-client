package se.bungalow.app.music;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;

import se.bungalow.library.LibraryEvents;

import se.bungalow.app.Application;
import se.bungalow.app.ApplicationContext;

import se.bungalow.app.music.concept.MusicConcept;

/**
 * Built-in plugin providing the music library: the data-backed views (tracks,
 * recordings, releases, artists, local files, detail pages, search, playlists)
 * registered into the {@code bungalow:library*} URI space, plus the "Your Library"
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
