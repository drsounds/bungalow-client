package se.spacify.app.catalogue.views;

import se.spacify.app.music.model.MusicRelease;
import se.spacify.navigation.ViewStack;
import se.spacify.app.catalogue.service.MusicCatalogueService;
import se.spacify.app.music.controls.MusicTable;

import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.util.List;

/**
 * Catalogue releases view. Populated by drilling in from an artist
 * ({@code ?artist=<mbid>} → browse the artist's releases) or by typing a search.
 * Double-clicking a release drills into its Recordings.
 */
public class ReleasesCatalogView extends AbstractCatalogView<MusicRelease> {

    public ReleasesCatalogView(ViewStack viewStack) { super(viewStack); }

    @Override protected String kind() { return "releases"; }
    @Override protected String searchHint() { return "Search releases…"; }
    @Override protected List<MusicTable.Column> getColumns() {
        return List.of(
            column("release", "Release"),
            column("date",    "Date"));
    }

    @Override
    protected List<MusicRelease> fetch(MusicCatalogueService svc) {
        String artist = param("artist");
        if (artist != null) return svc.browseReleasesByArtist(artist, 0, 100);
        return query().isBlank() ? List.of() : svc.searchReleases(query());
    }

    @Override
    protected MusicTable.Row toRow(MusicRelease r) {
        return row()
            .set("release", r.getName())
            .set("date",    r.getReleaseDate());
    }

    @Override
    protected void onActivate(int row) {
        MusicRelease r = rows.get(row);
        if (r.getMbid() != null) {
            open("recordings?release=" + URLEncoder.encode(r.getMbid(), StandardCharsets.UTF_8));
        }
    }

    @Override
    protected String emptyHeader(MusicCatalogueService svc) {
        if (param("artist") != null) return svc.getName() + " — no releases for this artist";
        return query().isBlank()
            ? "Pick an artist, or search " + svc.getName() + " releases"
            : svc.getName() + " — no releases for \"" + query() + "\"";
    }

    @Override public String getName() { return "Catalogue Releases"; }
}
