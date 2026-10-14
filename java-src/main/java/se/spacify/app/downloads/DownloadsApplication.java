package se.spacify.app.downloads;

import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;
import se.spacify.app.Application;
import se.spacify.app.ApplicationContext;
import se.spacify.app.downloads.concept.DownloadsConcept;

/**
 * Built-in plugin contributing the Download Manager. Its contributions — the
 * {@link DownloadService}, the audio capture scopes, the {@code spacify:downloads}
 * {@link DownloadsView} and the {@link se.spacify.app.downloads.model.Download}
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
