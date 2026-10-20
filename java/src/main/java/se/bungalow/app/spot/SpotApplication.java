package se.bungalow.app.spot;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;
import se.bungalow.app.Application;
import se.bungalow.app.ApplicationContext;
import se.bungalow.app.spot.chrome.Spot09Chrome;
import se.bungalow.app.spot.design.Spot09Design;
import se.bungalow.app.spot.skin.Spot09Skin;
import se.bungalow.app.spot.skin.Spot15Skin;
import se.bungalow.app.youtube.YouTubeMusicService;
 
/**
 * Built-in plugin contributing the YouTube streaming Service. Registers a
 * {@link YouTubeMusicService} (the streaming aspect,
 * {@link se.bungalow.app.media.service.MediaService}); registered after the local
 * music plugin so YouTube acts as a fallback for tracks not available locally.
 */
public class SpotApplication extends Application {

    @Override
    public void onActivate(ApplicationContext ctx) {
        ctx.registerChrome(new Spot09Chrome());
        ctx.registerDesign(new Spot09Design());
        ctx.registerSkin(new Spot09Skin());
        ctx.registerSkin(new Spot15Skin());
        ctx.registerTheme(new SpotTheme());
    }

    @Override
    public String getId() {
        // TODO Auto-generated method stub
        return "spot";
    }

    @Override
    public String getName() {
        // TODO Auto-generated method stub
        return "Spot";
    }

    @Override
    public void onRegister(AspectManager<? extends Aspect> aspectManager) {
        // TODO Auto-generated method stub
    }
}
