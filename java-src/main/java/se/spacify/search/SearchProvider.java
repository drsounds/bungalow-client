package se.spacify.search;

import java.util.List;

import se.spacify.aspect.Aspect;

/**
 * A pluggable source of search results — the app's "Searchable" contract. A
 * plugin implements this (and registers it via
 * {@code ApplicationContext.registerSearchProvider}) to contribute entities to the
 * unified search view: library entries, catalogue hits, YouTube videos, …
 *
 * <p>{@link #search} is invoked off the EDT and may perform I/O. {@code getName()}
 * (from {@link Aspect}) is the source label shown on each result.
 */
public interface SearchProvider extends Aspect {

    /** Results matching {@code query}; called off the EDT. Return empty, never null. */
    List<SearchResult> search(String query);
}
