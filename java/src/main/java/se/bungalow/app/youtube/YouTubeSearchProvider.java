package se.bungalow.app.youtube;

import java.util.ArrayList;
import java.util.List;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;
import se.bungalow.search.EntityKind;
import se.bungalow.search.SearchProvider;
import se.bungalow.search.SearchResult;

/**
 * Contributes YouTube video matches to the unified search view. Each result
 * navigates to {@code bungalow:youtube:<id>}, which the search view plays on the
 * YouTube service. Uses the Data API key when configured (via the service).
 */
public final class YouTubeSearchProvider implements SearchProvider {

    private static final int LIMIT = 8;

    private final YouTubeMusicService service;

    public YouTubeSearchProvider(YouTubeMusicService service) {
        this.service = service;
    }

    @Override public String getId()   { return "search.youtube"; }
    @Override public String getName() { return "YouTube"; }
    @Override public void onRegister(AspectManager<? extends Aspect> manager) {}

    @Override
    public List<SearchResult> search(String query) {
        List<SearchResult> out = new ArrayList<>();
        for (YouTubeSearch.Result r : YouTubeSearch.search(query, service.apiKey())) {
            out.add(new SearchResult(EntityKind.VIDEO, r.title(), getName(),
                "bungalow:youtube:" + r.videoId(), getName()));
            if (out.size() >= LIMIT) break;
        }
        return out;
    }
}
