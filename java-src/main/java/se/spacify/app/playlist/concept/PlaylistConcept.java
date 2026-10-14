package se.spacify.app.playlist.concept;


import java.util.ArrayList;
import java.util.List;

import javax.swing.Icon;

import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;
import se.spacify.concept.Concept;
import se.spacify.concept.ConceptContext;
import se.spacify.navigation.SidebarNode;
import se.spacify.navigation.ViewStack;
import se.spacify.app.Application;
import se.spacify.app.SidebarHandle;
import se.spacify.app.playlist.PlaylistEvents;
import se.spacify.app.playlist.service.PlaylistService;
import se.spacify.app.playlist.views.PlaylistView;

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
        ctx.registerEntity(se.spacify.app.playlist.model.Playlist.class);
        ctx.registerEntity(se.spacify.app.playlist.model.PlaylistRow.class);

        ctx.registerView(new PlaylistView(getViewStack()));

        SidebarNode root = new SidebarNode("Playlists", "spacify:playlists");
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
        nodes.add(new SidebarNode("＋ New Playlist", "spacify:playlist:new"));
        try {
            for (PlaylistService svc : getViewStack().getMainWindow().getServiceManager()
                    .getServices(PlaylistService.class)) {
                for (var pl : svc.getPlaylists())
                    nodes.add(new SidebarNode(pl.getName(), "spacify:playlist:" + pl.getPublicId()));
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
