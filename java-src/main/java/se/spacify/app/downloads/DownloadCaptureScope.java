package se.spacify.app.downloads;

import java.io.File;

import se.spacify.app.downloads.model.Download;

/**
 * A pluggable filter that claims certain downloads and handles them once
 * complete. Scopes are ordinary Services that also implement this interface, so
 * they are discovered via {@code ServiceManager.getServices(DownloadCaptureScope.class)}
 * — any plugin can add a new scope (audio, video, images, …). The audio scope is
 * contributed by the music/media domain and imports finished files into the
 * library.
 */
public interface DownloadCaptureScope {

    /** Short id recorded on the download (e.g. "music"). */
    String scopeId();

    /** Whether this scope wants to capture the given download. */
    boolean accepts(PendingDownload download);

    /**
     * Handle a completed file — e.g. read its metadata and create library
     * entries. Runs off the EDT. Implementations may set display metadata
     * (title/artist/album) on {@code row}; the manager persists it afterwards.
     */
    void onCompleted(File file, PendingDownload source, Download row) throws Exception;
}
