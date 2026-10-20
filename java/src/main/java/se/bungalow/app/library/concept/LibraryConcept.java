package se.bungalow.app.library.concept;

import java.util.ArrayList;
import java.util.List;

import javax.swing.Icon;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;
import se.bungalow.concept.Concept;
import se.bungalow.concept.ConceptContext;
import se.bungalow.db.DatabaseManager;
import se.bungalow.app.music.model.Artist;
import se.bungalow.app.music.model.MusicRelease;
import se.bungalow.library.LibraryEvents;
import se.bungalow.navigation.ViewStack;
import se.bungalow.navigation.SidebarNode;
import se.bungalow.app.Application;
import se.bungalow.app.SidebarHandle;
import se.bungalow.app.library.views.ArtistDetailView;
import se.bungalow.app.library.views.ArtistsLibraryView;
import se.bungalow.app.library.views.LocalFileLibraryView;
import se.bungalow.app.library.views.RecordingsLibraryView;
import se.bungalow.app.library.views.ReleaseDetailView;
import se.bungalow.app.library.views.ReleasesLibraryView;
import se.bungalow.app.library.views.TracksLibraryView;

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
        ctx.registerSearchProvider(new se.bungalow.app.library.LibrarySearchProvider());
        ctx.registerView(new TracksLibraryView(getViewStack()));
        ctx.registerView(new RecordingsLibraryView(getViewStack()));
        ctx.registerView(new ReleasesLibraryView(getViewStack()));
        ctx.registerView(new ArtistsLibraryView(getViewStack()));
        ctx.registerView(new LocalFileLibraryView(getViewStack()));
        ctx.registerView(new ReleaseDetailView(getViewStack()));
        ctx.registerView(new ArtistDetailView(getViewStack()));

        SidebarNode lib = new SidebarNode("Your Library", "bungalow:library");
        lib.addChild(new SidebarNode("Tracks",      "bungalow:library:tracks"));
        lib.addChild(new SidebarNode("Recordings",  "bungalow:library:recordings"));
        lib.addChild(new SidebarNode("Releases",    "bungalow:library:releases"));
        lib.addChild(new SidebarNode("Artists",     "bungalow:library:artists"));
        lib.addChild(new SidebarNode("Local Files", "bungalow:library:local"));

        SidebarHandle libHandle = ctx.addSidebarNode(lib);
        releases = libHandle.child("bungalow:library:releases");
        artists  = libHandle.child("bungalow:library:artists");
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
                out.add(new SidebarNode(r.getName(), "bungalow:library:release:" + r.getId()));
            }
        } catch (Exception ignored) {}
        return out;
    }

    private static List<SidebarNode> artistNodes() {
        List<SidebarNode> out = new ArrayList<>();
        try {
            for (Artist a : DatabaseManager.getInstance().dao(Artist.class).queryForAll()) {
                out.add(new SidebarNode(a.getName(), "bungalow:library:artist:" + a.getId()));
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
