package se.bungalow.app.web.concept;


import java.util.ArrayList;
import java.util.List;

import javax.swing.Icon;
import javax.swing.ImageIcon;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;
import se.bungalow.concept.Concept;
import se.bungalow.concept.ConceptContext;
import se.bungalow.navigation.SidebarNode;
import se.bungalow.navigation.ViewStack;
import se.bungalow.app.Application;
import se.bungalow.app.SidebarHandle;
import se.bungalow.app.web.model.Bookmark;
import se.bungalow.app.web.views.SPServiceWebView;
import se.bungalow.app.web.views.SPWebView;
import se.bungalow.web.BookmarkEvents;
import se.bungalow.web.BookmarkManager;

/**
 * The web-browsing concept: contributes the {@link SPWebView}/{@link SPServiceWebView}
 * into the {@code bungalow:} URI space and the "Sites" sidebar subtree of bookmarks,
 * kept in sync via {@link BookmarkEvents}. Owns the {@link Bookmark} table.
 */
public class WebConcept implements Concept {
    private Application plugin;
    public Application getApplication() {
        return plugin;
    }

    public ViewStack getViewStack() {
        return plugin.getViewStack();
    }

    public WebConcept(Application plugin) {
        this.plugin = plugin;
    }

    private final Runnable refresh = this::refresh;
    private SidebarHandle sites;

    @Override
    public void onActivate(ConceptContext ctx) {
        ctx.registerEntity(Bookmark.class);

        ctx.registerView(new SPWebView(getViewStack()));
        ctx.registerView(new SPServiceWebView(getViewStack()));

        sites = ctx.addSidebarNode(new SidebarNode("Sites", null));
        refresh();

        BookmarkEvents.addListener(refresh);
    }

    @Override
    public void onDeactivate() {
        BookmarkEvents.removeListener(refresh);
    }

    private void refresh() {
        if (sites != null) sites.setChildren(siteNodes());
    }

    private static List<SidebarNode> siteNodes() {
        List<SidebarNode> out = new ArrayList<>();
        for (Bookmark root : BookmarkManager.roots()) {
            SidebarNode rootNode = siteNode(root);
            for (Bookmark child : BookmarkManager.childrenOf(root)) {
                rootNode.addChild(siteNode(child));
            }
            out.add(rootNode);
        }
        return out;
    }

    private static SidebarNode siteNode(Bookmark b) {
        SidebarNode sn = new SidebarNode(b.toString(), b.getBungalowUri());
        if (b.getFavicon() != null) sn.setIcon(new ImageIcon(b.getFavicon()));
        return sn;
    }

    @Override
    public void onRegister(AspectManager<? extends Aspect> aspectManager) {
        // TODO Auto-generated method stub
    }

    @Override
    public String getId() {
        return "web";
    }

    @Override
    public String getName() {
        return "Web Browser";
    }

    @Override
    public Icon getIcon() {
        // TODO Auto-generated method stub
        throw new UnsupportedOperationException("Unimplemented method 'getIcon'");
    }

}
