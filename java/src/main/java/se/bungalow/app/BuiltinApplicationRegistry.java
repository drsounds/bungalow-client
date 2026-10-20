package se.bungalow.app;

import java.util.ArrayList;
import java.util.List;

/**
 * Applications compiled into the main bundle (no separate jar). They load with the
 * application class loader and provide the app's core content: local playback,
 * the music library, and in-app web browsing.
 */
final class BuiltinApplicationRegistry {

    private BuiltinApplicationRegistry() {}

    static List<ApplicationDescriptor> descriptors() {
        ClassLoader cl = BuiltinApplicationRegistry.class.getClassLoader();
        List<ApplicationDescriptor> list = new ArrayList<>();
        // Look-and-feel plugins: each contributes Designs (Chrome + Skin premixes)
        // selectable in Settings. Both enabled by default; WMP is the default Design.
        list.add(builtin("se.bungalow.app.wmp", "Windows Media Player", "1.0.0",
            "se.bungalow.app.wmp.WMPApplication", cl));
        list.add(builtin("se.bungalow.app.spot", "Spot", "1.0.0",
            "se.bungalow.app.spot.SpotApplication", cl));
        list.add(builtin("se.bungalow.app.localmusic", "Local Music", "1.0.0",
            "se.bungalow.app.localmusic.LocalMusicApplication", cl));
        list.add(builtin("se.bungalow.app.library", "Music Library", "1.0.0",
            "se.bungalow.app.library.LibraryApplication", cl));
        // After the library so its catalogue model is present; owns the Playlists
        // sidebar subtree and the editable local playlist store.
        list.add(builtin("se.bungalow.app.playlist", "Playlists", "1.0.0",
            "se.bungalow.app.playlist.PlaylistApplication", cl));
        list.add(builtin("se.bungalow.app.search", "Search", "1.0.0",
            "se.bungalow.app.search.SearchApplication", cl));
        list.add(builtin("se.bungalow.app.web", "Web & Sites", "1.0.0",
            "se.bungalow.app.web.WebApplication", cl));
        list.add(builtin("se.bungalow.app.musicbrainz", "MusicBrainz", "1.0.0",
            "se.bungalow.app.musicbrainz.MusicBrainzApplication", cl));
        // After MusicBrainz so the catalogue Services it registers are present
        // when the Catalogs sidebar folder is built.
        list.add(builtin("se.bungalow.app.catalogue", "Catalogs", "1.0.0",
            "se.bungalow.app.catalogue.CatalogueApplication", cl));
        // After local music so YouTube resolves as a fallback for tracks not held locally.
        list.add(builtin("se.bungalow.app.youtube", "YouTube", "1.0.0",
            "se.bungalow.app.youtube.YouTubeApplication", cl));
        // Download manager: captures audio downloads from the store/web views and
        // imports them into the library.
        list.add(builtin("se.bungalow.app.downloads", "Download Manager", "1.0.0",
            "se.bungalow.app.downloads.DownloadsApplication", cl));
        // Hello-world demo of the Spider template system (bungalow:testapp).
        list.add(builtin("se.bungalow.app.testapp", "Test App", "1.0.0",
            "se.bungalow.app.testapp.TestApplication", cl));
        return list;
    }

    private static ApplicationDescriptor builtin(String id, String name, String version,
                                            String mainClass, ClassLoader cl) {
        return new ApplicationDescriptor(id, name, version, mainClass,
            ApplicationDescriptor.Source.BUILTIN_BUNDLE, null, cl);
    }
}
