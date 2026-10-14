package se.spacify.app.localmusic.concept;


import javax.swing.Icon;

import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;
import se.spacify.concept.Concept;
import se.spacify.concept.ConceptContext;
import se.spacify.navigation.ViewStack;
import se.spacify.app.Application;
import se.spacify.app.localmusic.model.LocalFile;
import se.spacify.app.localmusic.service.LocalMusicService;

/**
 * The local-file playback concept: owns the {@link LocalFile} table and registers
 * the {@link LocalMusicService} (the music-streaming aspect) so the player bar,
 * now-playing queue and {@code PlaybackCoordinator} can resolve and play tracks.
 */
public class LocalMusicConcept implements Concept {
    private Application plugin;
    public Application getApplication() {
        return plugin;
    }

    public ViewStack getViewStack() {
        return plugin.getViewStack();
    }

    public LocalMusicConcept(Application plugin) {
        this.plugin = plugin;
    }

    @Override
    public void onActivate(ConceptContext ctx) {
        ctx.registerEntity(LocalFile.class);
        ctx.registerService(new LocalMusicService());
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
        return "localmusic";
    }

    @Override
    public String getName() {
        return "Local Music";
    }

    @Override
    public Icon getIcon() {
        // TODO Auto-generated method stub
        throw new UnsupportedOperationException("Unimplemented method 'getIcon'");
    }

}
