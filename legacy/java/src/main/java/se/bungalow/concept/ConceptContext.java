package se.bungalow.concept;

import se.bungalow.design.Design;
import se.bungalow.feature.Feature;
import se.bungalow.navigation.View;
import se.bungalow.navigation.ViewStack;
import se.bungalow.navigation.SidebarNode;
import se.bungalow.app.Application;
import se.bungalow.app.SidebarHandle;
import se.bungalow.service.Service;
import se.bungalow.skinning.Skin;
import se.bungalow.ui.chrome.Chrome;
import se.bungalow.ui.theme.Theme;

/**
 * Registration surface handed to {@link Application#onActivate}. Every contribution
 * made through this context is recorded, so disabling or removing the plugin
 * can undo exactly what it added (Services, features, views, sidebar nodes).
 */
public interface ConceptContext {

    /** This plugin's java-namespace id, e.g. {@code se.bungalow.app.library}. */
    String pluginId();

    /** Ensure this plugin's entity table exists (created if absent); returns its DAO. */
    <T> com.j256.ormlite.dao.Dao<T, Integer> registerEntity(Class<T> entity);

    void registerChrome(Chrome c);

    void registerDesign(Design d);
    void registerTheme(Theme t);

    void registerService(Service s);

    /** Register a Service (its {@code onCreate}/{@code onStart} are invoked). */
    void registerSkin(Skin s);

    /** Register a feature; its views and sidebar nodes are wired automatically. */
    void registerFeature(Feature f);

    /** Register a view into the {@code bungalow:} URI space. */
    void registerView(View v);

    /** Register a search provider; its results appear in the unified search view. */
    void registerSearchProvider(se.bungalow.search.SearchProvider p);

    /** Append a node (subtree) to the sidebar; the handle maintains dynamic children. */
    SidebarHandle addSidebarNode(SidebarNode n);

    /** The app view stack, for views (e.g. the web view) that drive navigation. */
    ViewStack viewStack();
}
