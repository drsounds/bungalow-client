package se.spacify.app.youtube;

import java.util.List;

import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;
import se.spacify.app.Application;
import se.spacify.app.ApplicationContext;
import se.spacify.app.ApplicationSetting;

/**
 * Built-in plugin contributing the YouTube streaming Service. Registers a
 * {@link YouTubeMusicService} (the streaming aspect,
 * {@link se.spacify.app.media.service.MediaService}); registered after the local
 * music plugin so YouTube acts as a fallback for tracks not available locally.
 */
public class YouTubeApplication extends Application {

    public YouTubeApplication() {
        super();
    }
    @Override
    public void onActivate(ApplicationContext ctx) {
        YouTubeMusicService service = new YouTubeMusicService(ctx.settings());
        ctx.registerService(service);
        ctx.registerSearchProvider(new YouTubeSearchProvider(service));
    }

    /** Optional YouTube Data API key; when set, search uses the API (else a scrape). */
    @Override
    public List<ApplicationSetting> getSettingsSchema() {
        return List.of(ApplicationSetting.string(YouTubeMusicService.SETTING_API_KEY,
            "YouTube Data API key (optional)", ""));
    }

    @Override
    public String getId() {
        // TODO Auto-generated method stub
        return "youtube";
    }

    @Override
    public String getName() {
        // TODO Auto-generated method stub
        return "YouTube";
    }

    @Override
    public void onRegister(AspectManager<? extends Aspect> aspectManager) {
        // TODO Auto-generated method stub
         
    }
}
