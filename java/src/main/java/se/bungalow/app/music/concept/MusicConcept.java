package se.bungalow.app.music.concept;


import javax.swing.Icon;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;
import se.bungalow.concept.Concept;
import se.bungalow.concept.ConceptContext;
import se.bungalow.navigation.ViewStack;
import se.bungalow.app.Application;
import se.bungalow.app.music.model.Artist;
import se.bungalow.app.music.model.MusicWork;
import se.bungalow.app.music.model.Recording;
import se.bungalow.app.music.model.RecordingFile;
import se.bungalow.app.music.model.MusicRelease;
import se.bungalow.app.music.model.Track;
import se.bungalow.app.music.model.RecordingCreatorCredit;
import se.bungalow.app.music.model.ReleaseCreatorCredit;

public class MusicConcept implements Concept {
    private Application plugin;
    public Application getApplication() {
        return plugin;
    }

    public ViewStack getViewStack() {
        return plugin.getViewStack();
    }

    public MusicConcept(Application plugin) {
        this.plugin = plugin;
    }
 
    @Override
    public void onActivate(ConceptContext ctx) {
        // The shared music/content model: this concept owns these tables; other
        // apps (library, catalogue, localmusic, media) read them through the DAO.
        ctx.registerEntity(Artist.class);
        ctx.registerEntity(MusicWork.class);
        ctx.registerEntity(Recording.class);
        ctx.registerEntity(RecordingFile.class);
        ctx.registerEntity(MusicRelease.class);
        ctx.registerEntity(Track.class);
        ctx.registerEntity(RecordingCreatorCredit.class);
        ctx.registerEntity(ReleaseCreatorCredit.class);
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
        // TODO Auto-generated method stub
        return "music";
    }

    @Override
    public String getName() {
        // TODO Auto-generated method stub
        return "Music";
    }

    @Override
    public Icon getIcon() {
        // TODO Auto-generated method stub
        throw new UnsupportedOperationException("Unimplemented method 'getIcon'");
    }

}
