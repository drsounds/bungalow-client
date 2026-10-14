package se.spacify.app.catalogue.views;

import se.spacify.app.music.model.Artist;
import se.spacify.navigation.ViewStack;
import se.spacify.app.catalogue.service.MusicCatalogueService;
import se.spacify.app.music.controls.MusicTable;

import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.util.List;

/**
 * Catalogue artists view — the searchable entry point into a remote catalogue.
 * Typing a query searches the catalogue's artists; double-clicking an artist
 * drills into that catalogue's Releases scoped to the artist.
 */
public class ArtistsCatalogView extends AbstractCatalogView<Artist> {

    public ArtistsCatalogView(ViewStack viewStack) { super(viewStack); }

    @Override protected String kind() { return "artists"; }
    @Override protected String searchHint() { return "Search artists…"; }
    @Override protected List<MusicTable.Column> getColumns() {
        return List.of(column("artist", "Artist"));
    }

    @Override
    protected List<Artist> fetch(MusicCatalogueService svc) {
        return query().isBlank() ? List.of() : svc.searchArtists(query());
    }

    @Override
    protected MusicTable.Row toRow(Artist a) {
        return row().set("artist", a.getName());
    }

    @Override
    protected void onActivate(int row) {
        Artist a = rows.get(row);
        if (a.getMbid() != null) {
            open("releases?artist=" + URLEncoder.encode(a.getMbid(), StandardCharsets.UTF_8));
        }
    }

    @Override
    protected String emptyHeader(MusicCatalogueService svc) {
        return query().isBlank()
            ? "Search the " + svc.getName() + " catalogue"
            : svc.getName() + " — no artists for \"" + query() + "\"";
    }

    @Override public String getName() { return "Catalogue Artists"; }
}
