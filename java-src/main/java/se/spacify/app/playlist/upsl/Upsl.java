package se.spacify.app.playlist.upsl;

/**
 * Parser for a Universal Playlist Sharing Link (UPSL) (RFC-0001 §5–6).
 * {@link #parse(String)} pulls the structural fields out of the link and
 * Base62-decodes the human-readable leaves ({@code name}, {@code description}) so
 * callers get plain strings.
 *
 * <p>Several starting forms are accepted; all continue identically from
 * {@code :playlist:} onward (id, then the optional body):
 *
 * <pre>{@code
 *   spacify:federated:user:<handle>:playlist:<uuid>:<name>:description:<desc>[:image:<img>]:uris:<refs>[:sig:<sig>]
 *   acct:<mastodon-user-id>:playlist:<uuid>:<name>:description:<desc>[:image:<img>]:uris:<refs>[:sig:<sig>]
 *   musik:acct:<mastodon-user-id>:playlist:<uuid>:…       (same body)
 *   bungalow:acct:<mastodon-user-id>:playlist:<uuid>:…    (same body)
 *   spacify:acct:<mastodon-user-id>:playlist:<uuid>:…     (same body)
 * }</pre>
 *
 * <p>The leading segment (the {@code handle}, or the {@code acct:}-family form's
 * mastodon user id) is exposed as {@link #handle()}. A bare link with no body
 * ({@code …:playlist:<uuid>}) is a <em>hosted</em> reference (§8): the handle and
 * playlist id are parsed, and {@code name}/{@code description} are empty. The
 * content-ref list and signature are out of scope for this parser; it reads only
 * what a preview needs.
 *
 * @see se.spacify.app.playlist.spec.RFC-0001-universal-playlist-sharing.md
 */
public final class Upsl {

    private static final String FEDERATED_PREFIX  = "spacify:federated:user:";
    private static final String ACCT_PREFIX       = "acct:";
    private static final String MUSIK_ACCT_PREFIX = "musik:acct:";
    private static final String BUNGALOW_ACCT_PREFIX = "bungalow:acct:";
    private static final String SPACIFY_ACCT_PREFIX  = "spacify:acct:";

    /** The prefixes that introduce a UPSL, longest-first so the more specific match wins. */
    private static final String[] PREFIXES = {
        FEDERATED_PREFIX, MUSIK_ACCT_PREFIX, BUNGALOW_ACCT_PREFIX, SPACIFY_ACCT_PREFIX, ACCT_PREFIX
    };
    private static final String PLAYLIST_MARKER = ":playlist:";
    private static final String DESCRIPTION_MARKER = ":description:";
    private static final String IMAGE_MARKER    = ":image:";
    private static final String URIS_MARKER     = ":uris:";

    private final String uri;
    private final String handle;
    private final String playlistId;
    private final String name;
    private final String description;

    private Upsl(String uri, String handle, String playlistId, String name, String description) {
        this.uri         = uri;
        this.handle      = handle;
        this.playlistId  = playlistId;
        this.name        = name;
        this.description = description;
    }

    /** The original link this was parsed from. */
    public String uri()         { return uri; }
    /** The issuer's {@code @user@instance} handle (§3). */
    public String handle()      { return handle; }
    /** The random UUIDv4 identifying the playlist across edits (§3). */
    public String playlistId()  { return playlistId; }
    /** The playlist display name, Base62-decoded; empty for a hosted link. */
    public String name()        { return name; }
    /** The playlist description, Base62-decoded; empty when absent. */
    public String description() { return description; }

    /** True if {@code uri} starts with one of the accepted UPSL prefixes (see the class javadoc). */
    public static boolean isUpsl(String uri) {
        return stripPrefix(uri) != null;
    }

    /**
     * Parse a UPSL in any of the accepted forms (see the class javadoc), or return
     * {@code null} if it isn't one / is malformed (missing the {@code :playlist:}
     * marker).
     *
     * @throws IllegalArgumentException if a Base62 leaf doesn't cleanly decode (§12).
     */
    public static Upsl parse(String uri) {
        String rest = stripPrefix(uri);
        if (rest == null) {
            return null;
        }

        int playlistAt = rest.indexOf(PLAYLIST_MARKER);
        if (playlistAt < 0) {
            return null;
        }
        String handle = rest.substring(0, playlistAt);
        String afterPlaylist = rest.substring(playlistAt + PLAYLIST_MARKER.length());

        // The playlist id is the next ':'-delimited token (a UUID has no colons).
        int idEnd = afterPlaylist.indexOf(':');
        String playlistId = idEnd < 0 ? afterPlaylist : afterPlaylist.substring(0, idEnd);
        String body = idEnd < 0 ? "" : afterPlaylist.substring(idEnd + 1);

        String name = "";
        String description = "";
        int descAt = body.indexOf(DESCRIPTION_MARKER);
        if (descAt >= 0) {
            // Body present: `<name>:description:<desc>[:image:…][:uris:…][:sig:…]`.
            name = Base62.decodeText(body.substring(0, descAt));
            String afterDesc = body.substring(descAt + DESCRIPTION_MARKER.length());
            description = Base62.decodeText(afterDesc.substring(0, descriptionEnd(afterDesc)));
        }
        return new Upsl(uri, handle, playlistId, name, description);
    }

    /** Strip whichever supported UPSL prefix {@code uri} carries, or {@code null} if it carries neither. */
    private static String stripPrefix(String uri) {
        if (uri == null) {
            return null;
        }
        for (String prefix : PREFIXES) {
            if (uri.startsWith(prefix)) {
                return uri.substring(prefix.length());
            }
        }
        return null;
    }

    /** The end of the description leaf: the first following marker, or end of string. */
    private static int descriptionEnd(String afterDesc) {
        int end = afterDesc.length();
        int image = afterDesc.indexOf(IMAGE_MARKER);
        int uris = afterDesc.indexOf(URIS_MARKER);
        if (image >= 0) end = Math.min(end, image);
        if (uris >= 0)  end = Math.min(end, uris);
        return end;
    }
}
