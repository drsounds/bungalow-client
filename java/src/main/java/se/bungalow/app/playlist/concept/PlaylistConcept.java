package se.bungalow.app.playlist.concept;


import java.util.ArrayList;
import java.util.List;

import javax.swing.Icon;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;
import se.bungalow.concept.Concept;
import se.bungalow.concept.ConceptContext;
import se.bungalow.navigation.SidebarNode;
import se.bungalow.navigation.ViewStack;
import se.bungalow.app.Application;
import se.bungalow.app.SidebarHandle;
import se.bungalow.app.playlist.PlaylistEvents;
import se.bungalow.app.playlist.service.PlaylistService;
import se.bungalow.app.playlist.views.PlaylistView;

public class PlaylistConcept implements Concept {
    private Application plugin;
    public Application getApplication() {
        return plugin;
    }

    public ViewStack getViewStack() {
        return plugin.getViewStack();
    }

    public PlaylistConcept(Application plugin) {
        this.plugin = plugin;
    }

    private final Runnable refresh = this::refresh;
    private SidebarHandle playlists;

    @Override
    public void onActivate(ConceptContext ctx) {
        ctx.registerEntity(se.bungalow.app.playlist.model.Playlist.class);
        ctx.registerEntity(se.bungalow.app.playlist.model.PlaylistRow.class);

        ctx.registerView(new PlaylistView(getViewStack()));

        SidebarNode root = new SidebarNode("Playlists", "bungalow:playlists");
        playlists = ctx.addSidebarNode(root);
        refresh();
        playlists.expand();

        PlaylistEvents.addListener(refresh);
    }

    @Override
    public void onDeactivate() {
        PlaylistEvents.removeListener(refresh);
    }

    /** Rebuild the Playlists subtree (a "New Playlist" entry plus one node per playlist). */
    private void refresh() {
        if (playlists == null) return;
        List<SidebarNode> nodes = new ArrayList<>();
        nodes.add(new SidebarNode("＋ New Playlist", "bungalow:playlist:new"));
        try {
            for (PlaylistService svc : getViewStack().getMainWindow().getServiceManager()
                    .getServices(PlaylistService.class)) {
                for (var pl : svc.getPlaylists())
                    nodes.add(new SidebarNode(pl.getName(), "bungalow:playlist:" + pl.getPublicId()));
            }
        } catch (Exception ignored) {}
        playlists.setChildren(nodes);
    }

    @Override
    public void onRegister(AspectManager<? extends Aspect> aspectManager) {
        // TODO Auto-generated method stub
    }

    @Override
    public String getId() {
        // TODO Auto-generated method stub
        return "playlist";
    }

    @Override
    public String getName() {
        // TODO Auto-generated method stub
        return "Playlist";
    }

    @Override
    public Icon getIcon() {
        // TODO Auto-generated method stub
        throw new UnsupportedOperationException("Unimplemented method 'getIcon'");
    }

}
