package se.bungalow.app.catalogue;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;
import se.bungalow.navigation.ViewStack;
import se.bungalow.navigation.SidebarNode;
import se.bungalow.app.Application;
import se.bungalow.app.ApplicationContext;
import se.bungalow.app.SidebarHandle;
import se.bungalow.app.catalogue.service.MusicCatalogueService;
import se.bungalow.app.catalogue.views.ArtistsCatalogView;
import se.bungalow.app.catalogue.views.RecordingsCatalogView;
import se.bungalow.app.catalogue.views.ReleasesCatalogView;

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
            node.addChild(new SidebarNode("Artists",    "bungalow:catalog:" + id + ":artists"));
            node.addChild(new SidebarNode("Releases",   "bungalow:catalog:" + id + ":releases"));
            node.addChild(new SidebarNode("Recordings", "bungalow:catalog:" + id + ":recordings"));
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
