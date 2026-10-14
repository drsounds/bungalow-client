package se.spacify.app.library.concept;

import java.util.ArrayList;
import java.util.List;

import javax.swing.Icon;

import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;
import se.spacify.concept.Concept;
import se.spacify.concept.ConceptContext;
import se.spacify.db.DatabaseManager;
import se.spacify.app.music.model.Artist;
import se.spacify.app.music.model.MusicRelease;
import se.spacify.library.LibraryEvents;
import se.spacify.navigation.ViewStack;
import se.spacify.navigation.SidebarNode;
import se.spacify.app.Application;
import se.spacify.app.SidebarHandle;
import se.spacify.app.library.views.ArtistDetailView;
import se.spacify.app.library.views.ArtistsLibraryView;
import se.spacify.app.library.views.LocalFileLibraryView;
import se.spacify.app.library.views.RecordingsLibraryView;
import se.spacify.app.library.views.ReleaseDetailView;
import se.spacify.app.library.views.ReleasesLibraryView;
import se.spacify.app.library.views.TracksLibraryView;

public class LibraryConcept implements Concept {
    private Application plugin;
    public Application getApplication() {
        return plugin;
    }

    public ViewStack getViewStack() {
        return plugin.getViewStack();
    }

    public LibraryConcept(Application plugin) {
        this.plugin = plugin;
    }

    private final Runnable refresh = this::refresh;
    private SidebarHandle releases;
    private SidebarHandle artists;
    @Override
    public void onActivate(ConceptContext ctx) {
        // The search view is owned by the search plugin now; the library only
        // contributes its entries as a search provider.
        ctx.registerSearchProvider(new se.spacify.app.library.LibrarySearchProvider());
        ctx.registerView(new TracksLibraryView(getViewStack()));
        ctx.registerView(new RecordingsLibraryView(getViewStack()));
        ctx.registerView(new ReleasesLibraryView(getViewStack()));
        ctx.registerView(new ArtistsLibraryView(getViewStack()));
        ctx.registerView(new LocalFileLibraryView(getViewStack()));
        ctx.registerView(new ReleaseDetailView(getViewStack()));
        ctx.registerView(new ArtistDetailView(getViewStack()));

        SidebarNode lib = new SidebarNode("Your Library", "spacify:library");
        lib.addChild(new SidebarNode("Tracks",      "spacify:library:tracks"));
        lib.addChild(new SidebarNode("Recordings",  "spacify:library:recordings"));
        lib.addChild(new SidebarNode("Releases",    "spacify:library:releases"));
        lib.addChild(new SidebarNode("Artists",     "spacify:library:artists"));
        lib.addChild(new SidebarNode("Local Files", "spacify:library:local"));

        SidebarHandle libHandle = ctx.addSidebarNode(lib);
        releases = libHandle.child("spacify:library:releases");
        artists  = libHandle.child("spacify:library:artists");
        refresh();
        libHandle.expand();

        LibraryEvents.addListener(refresh);
    }

    @Override
    public void onDeactivate() {
        LibraryEvents.removeListener(refresh);
    }

    private void refresh() {
        if (releases != null) releases.setChildren(releaseNodes());
        if (artists  != null) artists.setChildren(artistNodes());
    }

    private static List<SidebarNode> releaseNodes() {
        List<SidebarNode> out = new ArrayList<>();
        try {
            for (MusicRelease r : DatabaseManager.getInstance().dao(MusicRelease.class).queryForAll()) {
                out.add(new SidebarNode(r.getName(), "spacify:library:release:" + r.getId()));
            }
        } catch (Exception ignored) {}
        return out;
    }

    private static List<SidebarNode> artistNodes() {
        List<SidebarNode> out = new ArrayList<>();
        try {
            for (Artist a : DatabaseManager.getInstance().dao(Artist.class).queryForAll()) {
                out.add(new SidebarNode(a.getName(), "spacify:library:artist:" + a.getId()));
            }
        } catch (Exception ignored) {}
        return out;
    }
 
    @Override
    public void onRegister(AspectManager<? extends Aspect> aspectManager) {
        // TODO Auto-generated method stub
    }
 
    @Override
    public String getId() {
        // TODO Auto-generated method stub
        return "library";
    }

    @Override
    public String getName() {
        // TODO Auto-generated method stub
        return "Library";
    }

    @Override
    public Icon getIcon() {
        // TODO Auto-generated method stub
        throw new UnsupportedOperationException("Unimplemented method 'getIcon'");
    }

}
