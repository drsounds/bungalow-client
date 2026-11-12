package se.bungalow.app.search;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;
import se.bungalow.app.Application;
import se.bungalow.app.ApplicationContext;
import se.bungalow.app.search.views.SearchView;

/**
 * Built-in plugin owning the unified search screen ({@code bungalow:search}). The
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
