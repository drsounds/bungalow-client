package se.bungalow.app.downloads;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;
import se.bungalow.app.Application;
import se.bungalow.app.ApplicationContext;
import se.bungalow.app.downloads.concept.DownloadsConcept;

/**
 * Built-in plugin contributing the Download Manager. Its contributions — the
 * {@link DownloadService}, the audio capture scopes, the {@code bungalow:downloads}
 * {@link DownloadsView} and the {@link se.bungalow.app.downloads.model.Download}
 * table — are carried by the {@link DownloadsConcept}.
 */
public class DownloadsApplication extends Application {

    @Override public String getId()   { return "downloads"; }
    @Override public String getName() { return "Download Manager"; }
    @Override public void onRegister(AspectManager<? extends Aspect> manager) {}

    @Override
    public void onActivate(ApplicationContext ctx) {
        ctx.registerConcept(new DownloadsConcept(this));
    }
}
