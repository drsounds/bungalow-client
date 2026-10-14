package se.spacify.app.web.concept;


import java.util.ArrayList;
import java.util.List;

import javax.swing.Icon;
import javax.swing.ImageIcon;

import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;
import se.spacify.concept.Concept;
import se.spacify.concept.ConceptContext;
import se.spacify.navigation.SidebarNode;
import se.spacify.navigation.ViewStack;
import se.spacify.app.Application;
import se.spacify.app.SidebarHandle;
import se.spacify.app.web.model.Bookmark;
import se.spacify.app.web.views.SPServiceWebView;
import se.spacify.app.web.views.SPWebView;
import se.spacify.web.BookmarkEvents;
import se.spacify.web.BookmarkManager;

/**
 * The web-browsing concept: contributes the {@link SPWebView}/{@link SPServiceWebView}
 * into the {@code spacify:} URI space and the "Sites" sidebar subtree of bookmarks,
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
        SidebarNode sn = new SidebarNode(b.toString(), b.getSpacifyUri());
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
