package se.spacify.app.localmusic;

import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;
import se.spacify.app.Application;
import se.spacify.app.ApplicationContext;
import se.spacify.app.localmusic.concept.LocalMusicConcept;

/**
 * Built-in plugin providing local file playback. Its contributions — the
 * {@link se.spacify.app.localmusic.service.LocalMusicService} and the
 * {@link se.spacify.app.localmusic.model.LocalFile} table — are carried by the
 * {@link LocalMusicConcept}.
 */
public class LocalMusicApplication extends Application {

    @Override
    public void onActivate(ApplicationContext ctx) {
        ctx.registerConcept(new LocalMusicConcept(this));
    }

    @Override
    public String getId() {
        return "localmusic";
    }

    @Override
    public String getName() {
        return "Local Music";
    }

    @Override
    public void onRegister(AspectManager<? extends Aspect> aspectManager) {
        // TODO Auto-generated method stub
    }
}
