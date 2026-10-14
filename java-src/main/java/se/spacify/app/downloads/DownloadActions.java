package se.spacify.app.downloads;

/**
 * Broadcast action names published by the download manager. Any component
 * (the Downloads view, a header spinner, a menu badge) can subscribe via
 * {@link se.spacify.broadcast.BroadcastManager} without depending on this plugin.
 */
public final class DownloadActions {
    private DownloadActions() {}

    /** The download list changed; extra {@code "active"} = number in progress. */
    public static final String CHANGED   = "bungalow.downloads.changed";
    /** A download finished; extra {@code "id"} = the Download row id. */
    public static final String COMPLETED = "bungalow.downloads.completed";
}
