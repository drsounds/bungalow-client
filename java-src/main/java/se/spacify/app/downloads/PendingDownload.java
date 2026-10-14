package se.spacify.app.downloads;

/**
 * A download about to start, offered to every {@link DownloadCaptureScope} so a
 * scope can decide whether to claim it. Derived from the JCEF download item.
 */
public record PendingDownload(String url, String suggestedName, String mimeType, long totalBytes) {

    /** Lower-cased file extension of the suggested name (without the dot), or "". */
    public String extension() {
        if (suggestedName == null) return "";
        int dot = suggestedName.lastIndexOf('.');
        return dot >= 0 ? suggestedName.substring(dot + 1).toLowerCase() : "";
    }
}
