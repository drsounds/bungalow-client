package se.spacify.app.catalogue;

import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;
import se.spacify.navigation.ViewStack;
import se.spacify.navigation.SidebarNode;
import se.spacify.app.Application;
import se.spacify.app.ApplicationContext;
import se.spacify.app.SidebarHandle;
import se.spacify.app.catalogue.service.MusicCatalogueService;
import se.spacify.app.catalogue.views.ArtistsCatalogView;
import se.spacify.app.catalogue.views.RecordingsCatalogView;
import se.spacify.app.catalogue.views.ReleasesCatalogView;

/**
 * Built-in plugin contributing the <strong>Catalogs</strong> sidebar folder and
 * the catalogue browsing views. Every registered
 * {@link MusicCatalogueService} (MusicBrainz, …) becomes a node with
 * Artists / Releases / Recordings sub-entries that open the shared, async
 * catalogue views — browsing remote data on background threads, in the spirit of
 * Windows Media Player 11's "Urge" online catalogue.
 */
public class CatalogueApplication extends Application {

    @Override
    public void onActivate(ApplicationContext ctx) {
        ViewStack viewStack = ctx.viewStack();
        ctx.registerView(new ArtistsCatalogView(viewStack));
        ctx.registerView(new ReleasesCatalogView(viewStack));
        ctx.registerView(new RecordingsCatalogView(viewStack));
        ctx.registerSearchProvider(new CatalogueSearchProvider());

        SidebarNode catalogs = new SidebarNode("Catalogs", null);
        for (MusicCatalogueService svc : getManager().getMainWindow().getServiceManager().getServices(MusicCatalogueService.class)) {
            String id = svc.getId();
            SidebarNode node = new SidebarNode(svc.getName(), null);
            node.addChild(new SidebarNode("Artists",    "spacify:catalog:" + id + ":artists"));
            node.addChild(new SidebarNode("Releases",   "spacify:catalog:" + id + ":releases"));
            node.addChild(new SidebarNode("Recordings", "spacify:catalog:" + id + ":recordings"));
            catalogs.addChild(node);
        }

        SidebarHandle handle = ctx.addSidebarNode(catalogs);
        handle.expand();
    }

    @Override
    public String getId() {
        // TODO Auto-generated method stub
        return "catalogs";
    }

    @Override
    public String getName() {
        // TODO Auto-generated method stub
        return "Catalogs";
    }

    @Override
    public void onRegister(AspectManager<? extends Aspect> aspectManager) {
        // TODO Auto-generated method stub
        return;
    }
}
