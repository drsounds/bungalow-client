package se.bungalow.app.localmusic;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;
import se.bungalow.app.Application;
import se.bungalow.app.ApplicationContext;
import se.bungalow.app.localmusic.concept.LocalMusicConcept;

/**
 * Built-in plugin providing local file playback. Its contributions — the
 * {@link se.bungalow.app.localmusic.service.LocalMusicService} and the
 * {@link se.bungalow.app.localmusic.model.LocalFile} table — are carried by the
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
