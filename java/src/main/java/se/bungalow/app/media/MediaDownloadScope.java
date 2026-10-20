package se.bungalow.app.media;

import java.io.File;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;
import se.bungalow.app.downloads.model.Download;
import se.bungalow.library.MusicScanner;
import se.bungalow.app.downloads.DownloadCaptureScope;
import se.bungalow.app.downloads.PendingDownload;
import se.bungalow.service.Service;

/**
 * Media download capture scope: the broad {@code audio/*} catch-all, claiming
 * audio downloads the narrower music scope didn't match by extension (e.g. an
 * audio content-type with an unusual name). Imports via {@link MusicScanner}.
 * Together with {@link se.bungalow.app.music.MusicDownloadScope} this forms a
 * layered filter — specific first, broad fallback second.
 */
public final class MediaDownloadScope implements Service, DownloadCaptureScope {

    @Override public String scopeId() { return "media"; }

    @Override
    public boolean accepts(PendingDownload download) {
        String mime = download.mimeType();
        return mime != null && mime.toLowerCase().startsWith("audio/");
    }

    @Override
    public void onCompleted(File file, PendingDownload source, Download row) throws Exception {
        MusicScanner.Imported m = new MusicScanner().importFile(file);
        row.setName(m.title());
        row.setArtist(m.artist());
        row.setAlbum(m.album());
    }

    // ── Service / Aspect ──────────────────────────────────────────────────────────
    @Override public String getId()   { return "media-download-scope"; }
    @Override public String getName() { return "Media Downloads"; }
    @Override public void onRegister(AspectManager<? extends Aspect> manager) {}
}
