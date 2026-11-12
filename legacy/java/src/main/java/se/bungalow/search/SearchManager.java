package se.bungalow.search;

import java.util.Collection;

import se.bungalow.aspect.BaseAspectManager;
import se.bungalow.ui.MainWindow;

/**
 * Registry of the {@link SearchProvider}s contributed by plugins. The search view
 * queries every provider and groups their {@link SearchResult}s by
 * {@link EntityKind}.
 */
public class SearchManager extends BaseAspectManager<SearchProvider> {

    public SearchManager(MainWindow mainWindow) {
        super(mainWindow);
    }

    /** All registered providers, in registration order. */
    public Collection<SearchProvider> providers() {
        return all();
    }
}
