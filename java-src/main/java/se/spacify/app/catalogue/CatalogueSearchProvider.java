package se.spacify.app.catalogue;

import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;
import se.spacify.db.LibraryRepository;
import se.spacify.app.music.model.Artist;
import se.spacify.app.music.model.MusicRelease;
import se.spacify.app.catalogue.service.MusicCatalogueService;
import se.spacify.search.EntityKind;
import se.spacify.search.SearchProvider;
import se.spacify.search.SearchResult;
import se.spacify.ui.MainWindow;

/**
 * Searches every registered {@link MusicCatalogueService} (MusicBrainz, …) for
 * artists and releases. Results drill into the catalogue browse views
 * ({@code spacify:catalog:<svc>:releases?artist=…} / {@code …:recordings?release=…}).
 */
public final class CatalogueSearchProvider implements SearchProvider {

    private static final int LIMIT = 6;

    @Override public String getId()   { return "search.catalogue"; }
    @Override public String getName() { return "Catalogue"; }
    @Override public void onRegister(AspectManager<? extends Aspect> manager) {}

    @Override
    public List<SearchResult> search(String query) {
        List<SearchResult> out = new ArrayList<>();
        MainWindow mw = MainWindow.getInstance();
        if (mw == null) return out;
        for (MusicCatalogueService svc : mw.getServiceManager().getServices(MusicCatalogueService.class)) {
            try {
                int n = 0;
                for (Artist a : svc.searchArtists(query)) {
                    if (a.getMbid() == null || n++ >= LIMIT) break;
                    out.add(new SearchResult(EntityKind.ARTIST, a.getName(), svc.getName(),
                        "spacify:catalog:" + svc.getId() + ":releases?artist=" + enc(a.getMbid()), svc.getName()));
                }
                n = 0;
                for (MusicRelease r : svc.searchReleases(query)) {
                    if (r.getMbid() == null || n++ >= LIMIT) break;
                    out.add(new SearchResult(EntityKind.RELEASE, r.getName(),
                        LibraryRepository.artistNamesForRelease(r),
                        "spacify:catalog:" + svc.getId() + ":recordings?release=" + enc(r.getMbid()), svc.getName()));
                }
            } catch (Exception ignored) {
                // a catalogue failing shouldn't sink the others.
            }
        }
        return out;
    }

    private static String enc(String s) {
        return URLEncoder.encode(s, StandardCharsets.UTF_8);
    }
}
