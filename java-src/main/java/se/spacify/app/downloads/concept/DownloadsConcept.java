package se.spacify.app.downloads.concept;


import javax.swing.Icon;

import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;
import se.spacify.concept.Concept;
import se.spacify.concept.ConceptContext;
import se.spacify.navigation.ViewStack;
import se.spacify.app.Application;
import se.spacify.app.downloads.DownloadService;
import se.spacify.app.downloads.DownloadsView;
import se.spacify.app.downloads.model.Download;
import se.spacify.app.media.MediaDownloadScope;
import se.spacify.app.music.MusicDownloadScope;

/**
 * The download-manager concept: owns the {@link Download} table, registers the
 * {@link DownloadService} plus the built-in audio capture scopes, and the
 * {@code spacify:downloads} {@link DownloadsView}. The music scope (specific
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
