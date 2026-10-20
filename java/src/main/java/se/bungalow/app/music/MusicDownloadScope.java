package se.bungalow.app.music;

import java.io.File;
import java.util.Set;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;
import se.bungalow.app.downloads.model.Download;
import se.bungalow.library.MusicScanner;
import se.bungalow.app.downloads.DownloadCaptureScope;
import se.bungalow.app.downloads.PendingDownload;
import se.bungalow.service.Service;

/**
 * Music download capture scope: claims the specific audio file types by
 * extension and imports a finished file into the library via {@link MusicScanner}.
 * The narrower of the audio scopes — checked before the media plugin's
 * {@code audio/*} catch-all.
 */
public final class MusicDownloadScope implements Service, DownloadCaptureScope {

    private static final Set<String> EXTENSIONS = Set.of("mp3", "wav", "aac", "flac", "ogg");

    @Override public String scopeId() { return "music"; }

    @Override
    public boolean accepts(PendingDownload download) {
        return EXTENSIONS.contains(download.extension());
    }

    @Override
    public void onCompleted(File file, PendingDownload source, Download row) throws Exception {
        MusicScanner.Imported m = new MusicScanner().importFile(file);
        row.setName(m.title());
        row.setArtist(m.artist());
        row.setAlbum(m.album());
    }

    // ── Service / Aspect ──────────────────────────────────────────────────────────
    @Override public String getId()   { return "music-download-scope"; }
    @Override public String getName() { return "Music Downloads"; }
    @Override public void onRegister(AspectManager<? extends Aspect> manager) {}
}
