package se.spacify.app.search;

import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;
import se.spacify.app.Application;
import se.spacify.app.ApplicationContext;
import se.spacify.app.search.views.SearchView;

/**
 * Built-in plugin owning the unified search screen ({@code spacify:search}). The
 * view aggregates every registered {@code SearchProvider}; other plugins
 * contribute their entities by registering providers (the "Searchable" model).
 */
public class SearchApplication extends Application {

    @Override public String getId()   { return "search"; }
    @Override public String getName() { return "Search"; }
    @Override public void onRegister(AspectManager<? extends Aspect> aspectManager) {}

    @Override
    public void onActivate(ApplicationContext ctx) {
        ctx.registerView(new SearchView(ctx.viewStack()));
    }
}
