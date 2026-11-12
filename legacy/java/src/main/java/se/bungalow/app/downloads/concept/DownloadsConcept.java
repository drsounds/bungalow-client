package se.bungalow.app.downloads.concept;


import javax.swing.Icon;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;
import se.bungalow.concept.Concept;
import se.bungalow.concept.ConceptContext;
import se.bungalow.navigation.ViewStack;
import se.bungalow.app.Application;
import se.bungalow.app.downloads.DownloadService;
import se.bungalow.app.downloads.DownloadsView;
import se.bungalow.app.downloads.model.Download;
import se.bungalow.app.media.MediaDownloadScope;
import se.bungalow.app.music.MusicDownloadScope;

/**
 * The download-manager concept: owns the {@link Download} table, registers the
 * {@link DownloadService} plus the built-in audio capture scopes, and the
 * {@code bungalow:downloads} {@link DownloadsView}. The music scope (specific
 * extensions) is registered before the media scope (broad {@code audio/*}) so the
 * narrower filter matches first.
 */
public class DownloadsConcept implements Concept {
    private Application plugin;
    public Application getApplication() {
        return plugin;
    }

    public ViewStack getViewStack() {
        return plugin.getViewStack();
    }

    public DownloadsConcept(Application plugin) {
        this.plugin = plugin;
    }

    @Override
    public void onActivate(ConceptContext ctx) {
        ctx.registerEntity(Download.class);
        ctx.registerService(new DownloadService());
        ctx.registerService(new MusicDownloadScope());   // specific audio extensions (checked first)
        ctx.registerService(new MediaDownloadScope());   // broad audio/* fallback
        ctx.registerView(new DownloadsView(ctx.viewStack()));
    }

    @Override
    public void onDeactivate() {
    }

    @Override
    public void onRegister(AspectManager<? extends Aspect> aspectManager) {
        // TODO Auto-generated method stub
    }

    @Override
    public String getId() {
        return "downloads";
    }

    @Override
    public String getName() {
        return "Download Manager";
    }

    @Override
    public Icon getIcon() {
        // TODO Auto-generated method stub
        throw new UnsupportedOperationException("Unimplemented method 'getIcon'");
    }

}
