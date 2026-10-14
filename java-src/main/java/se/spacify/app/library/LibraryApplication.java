package se.spacify.app.library;

import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;
import se.spacify.library.LibraryEvents;
import se.spacify.app.Application;
import se.spacify.app.ApplicationContext;
import se.spacify.app.library.concept.LibraryConcept;

/**
 * Built-in plugin providing the music library: the data-backed views (tracks,
 * recordings, releases, artists, local files, detail pages, search, playlists)
 * registered into the {@code spacify:library*} URI space, plus the "Your Library"
 * sidebar subtree with live Releases/Artists lists kept in sync via
 * {@link LibraryEvents}.
 */
public class LibraryApplication extends Application {


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
        ctx.registerConcept(new LibraryConcept(this));
    }
}
