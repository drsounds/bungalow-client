package se.spacify.app.music.net;

import java.net.URLDecoder;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.util.Locale;

import se.spacify.service.media.PlayRequest;

/**
 * Parser, builder and playback resolver for the {@code musik:} URI scheme — the
 * canonical, service-agnostic way to name a piece of music by a standard
 * identifier (ISRC/ISWC/UPC/ISNI/IPI) or by human-readable coordinates
 * (artist / release / track / name / version). See
 * {@code src/main/java/se/spacify/app/music/spec/RFC-0002-musik-uri-scheme.md}.
 *
 * <p>This is the music app's URI resolver: {@link #parse(String)} turns a URI
 * into a typed value, {@link #toString()} renders one back (round-tripping), and
 * {@link #toPlayRequest(String)} produces a vendor-neutral {@link PlayRequest} for
 * the playable forms so the rest of the app (playlist rows, drag payloads, direct
 * plays) resolves {@code musik:} identity through the normal
 * {@code PlaybackCoordinator} path.
 *
 * <p>The predecessor content URI {@code spacify:recording:isrc:<ISRC>} is still
 * accepted and treated as equivalent to {@code musik:isrc:<ISRC>} (RFC-0002 §9).
 */
public final class MusikUri {

    public static final String SCHEME              = "musik:";
    /** Legacy recording content URI, equivalent to {@code musik:isrc:} (RFC-0002 §9). */
    public static final String LEGACY_ISRC_PREFIX  = "spacify:recording:isrc:";

    /** The entity a {@code musik:} URI names. */
    public enum Kind {
        ISRC, ISWC, UPC, ISNI, IPI, GENRE, MOOD, ARTIST, RELEASE, RECORDING
    }

    private final Kind   kind;
    private final String id;        // identifier value for the id forms; else null
    private final String artist;    // name-form coordinates (null when unused)
    private final String release;
    private final String recording;
    private final String version;
    private final int    trackNumber; // 0 when unspecified

    private MusikUri(Kind kind, String id, String artist, String release,
                     String recording, String version, int trackNumber) {
        this.kind        = kind;
        this.id          = id;
        this.artist      = artist;
        this.release     = release;
        this.recording   = recording;
        this.version     = version;
        this.trackNumber = trackNumber;
    }

    // ── Factories ────────────────────────────────────────────────────────────

    private static MusikUri idForm(Kind kind, String value) {
        return new MusikUri(kind, value, null, null, null, null, 0);
    }

    public static MusikUri isrc(String isrc)  { return idForm(Kind.ISRC, normalizeId(isrc)); }
    public static MusikUri iswc(String iswc)  { return idForm(Kind.ISWC, normalizeId(iswc)); }
    public static MusikUri upc(String upc)    { return idForm(Kind.UPC,  normalizeId(upc)); }
    public static MusikUri isni(String isni)  { return idForm(Kind.ISNI, normalizeId(isni)); }
    public static MusikUri ipi(String ipi)    { return idForm(Kind.IPI,  normalizeId(ipi)); }
    public static MusikUri genre(String name) { return idForm(Kind.GENRE, slug(name)); }
    public static MusikUri mood(String name)  { return idForm(Kind.MOOD,  slug(name)); }

    public static MusikUri artist(String artist) {
        return new MusikUri(Kind.ARTIST, null, artist, null, null, null, 0);
    }

    public static MusikUri release(String artist, String release) {
        return new MusikUri(Kind.RELEASE, null, artist, release, null, null, 0);
    }

    /**
     * A recording addressed by name coordinates. {@code release}/{@code version}
     * may be null and {@code trackNumber} may be 0 (unspecified); a null release
     * renders via the {@code musik:track?…} query form (RFC-0002 §6–7).
     */
    public static MusikUri recording(String artist, String release, int trackNumber,
                                     String name, String version) {
        return new MusikUri(Kind.RECORDING, null, artist, release, name, version,
                clampTrackNumber(trackNumber));
    }

    // ── Accessors ────────────────────────────────────────────────────────────

    public Kind   kind()        { return kind; }
    public String id()          { return id; }
    public String artist()      { return artist; }
    public String release()     { return release; }
    public String recording()   { return recording; }
    public String version()     { return version; }
    public int    trackNumber() { return trackNumber; }

    // ── Parsing ──────────────────────────────────────────────────────────────

    /** True if {@code uri} is a {@code musik:} or legacy recording content URI. */
    public static boolean isContentUri(String uri) {
        return uri != null && (uri.startsWith(SCHEME) || uri.startsWith(LEGACY_ISRC_PREFIX));
    }

    /**
     * Parse a {@code musik:} URI (or the legacy {@code spacify:recording:isrc:}
     * form) into a typed value, or {@code null} if it isn't one / is malformed.
     */
    public static MusikUri parse(String uri) {
        if (uri == null) return null;
        String s = uri.trim();
        if (s.startsWith(LEGACY_ISRC_PREFIX)) {
            String v = normalizeId(s.substring(LEGACY_ISRC_PREFIX.length()));
            return v.isEmpty() ? null : idForm(Kind.ISRC, v);
        }
        if (!s.startsWith(SCHEME)) return null;
        String rest = s.substring(SCHEME.length());
        if (rest.isEmpty()) return null;

        if (rest.startsWith("track?")) return parseQuery(rest.substring("track?".length()));
        if (rest.startsWith("artist:")) return parseName(rest);
        return parseIdForm(rest);
    }

    private static MusikUri parseIdForm(String rest) {
        int colon = rest.indexOf(':');
        if (colon <= 0 || colon == rest.length() - 1) return null;
        String key = rest.substring(0, colon).toLowerCase(Locale.ROOT);
        String raw = rest.substring(colon + 1);
        switch (key) {
            case "isrc": return idForm(Kind.ISRC, normalizeId(raw));
            case "iswc": return idForm(Kind.ISWC, normalizeId(raw));
            case "upc":  return idForm(Kind.UPC,  normalizeId(raw));
            case "isni": return idForm(Kind.ISNI, normalizeId(raw));
            case "ipi":  return idForm(Kind.IPI,  normalizeId(raw));
            case "genre": return idForm(Kind.GENRE, pctDecode(raw));
            case "mood":  return idForm(Kind.MOOD,  pctDecode(raw));
            default: return null;
        }
    }

    private static MusikUri parseName(String rest) {
        String[] segs = rest.split(":", -1);
        // segs[0] == "artist", segs[1] == artist value
        if (segs.length < 2 || segs[1].isEmpty()) return null;
        String artist = pctDecode(segs[1]);
        String release = null, name = null, version = null;
        int number = 0;
        for (int i = 2; i + 1 < segs.length; i += 2) {
            String key = segs[i].toLowerCase(Locale.ROOT);
            String val = segs[i + 1];
            switch (key) {
                case "release": release = pctDecode(val); break;
                case "track":   number  = parseTrackNumber(val); break;
                case "name":    name    = pctDecode(val); break;
                case "version": version = pctDecode(val); break;
                default: /* ignore unknown segment (forward-compat) */ break;
            }
        }
        if (name != null)    return recording(artist, release, number, name, version);
        if (release != null) return release(artist, release);
        return artist(artist);
    }

    private static MusikUri parseQuery(String query) {
        String artist = null, release = null, name = null, version = null;
        int number = 0;
        for (String param : query.split("&")) {
            int eq = param.indexOf('=');
            if (eq < 0) continue;
            String key = param.substring(0, eq).toLowerCase(Locale.ROOT);
            String val = formDecode(param.substring(eq + 1));
            switch (key) {
                case "name":         name    = val; break;
                case "artist_name":  artist  = val; break;
                case "release_name": release = val; break;
                case "number":       number  = parseTrackNumber(val); break;
                case "version":      version = val; break;
                default: break;
            }
        }
        if (name == null || name.isEmpty()) return null;   // name is REQUIRED (§7)
        return recording(artist, release, number, name, version);
    }

    // ── Rendering ────────────────────────────────────────────────────────────

    @Override
    public String toString() {
        switch (kind) {
            case ISRC:  return SCHEME + "isrc:" + id;
            case ISWC:  return SCHEME + "iswc:" + id;
            case UPC:   return SCHEME + "upc:"  + id;
            case ISNI:  return SCHEME + "isni:" + id;
            case IPI:   return SCHEME + "ipi:"  + id;
            case GENRE: return SCHEME + "genre:" + id;
            case MOOD:  return SCHEME + "mood:"  + id;
            case ARTIST:
                return SCHEME + "artist:" + enc(artist);
            case RELEASE:
                return SCHEME + "artist:" + enc(artist) + ":release:" + enc(release);
            case RECORDING:
                if (release == null) return toQueryUri();   // grammar nests name under release
                StringBuilder sb = new StringBuilder(SCHEME)
                        .append("artist:").append(enc(artist))
                        .append(":release:").append(enc(release));
                if (trackNumber > 0) sb.append(":track:").append(trackNumber);
                sb.append(":name:").append(enc(recording));
                if (version != null && !version.isEmpty()) sb.append(":version:").append(enc(version));
                return sb.toString();
            default:
                throw new IllegalStateException("unhandled kind " + kind);
        }
    }

    /** The equivalent {@code musik:track?…} query-form URI (RFC-0002 §7). */
    public String toQueryUri() {
        StringBuilder sb = new StringBuilder(SCHEME).append("track?name=").append(enc(recording));
        if (artist  != null && !artist.isEmpty())  sb.append("&artist_name=").append(enc(artist));
        if (release != null && !release.isEmpty())  sb.append("&release_name=").append(enc(release));
        if (trackNumber > 0)                        sb.append("&number=").append(trackNumber);
        if (version != null && !version.isEmpty())  sb.append("&version=").append(enc(version));
        return sb.toString();
    }

    // ── Playback resolution ──────────────────────────────────────────────────

    /**
     * A vendor-neutral {@link PlayRequest} for a playable {@code musik:} URI, or
     * {@code null} if the URI isn't a directly playable recording (an artist,
     * release, work, facet, or non-{@code musik:} URI). The request carries the
     * original URI as its {@code fallbackUri} so it round-trips through the queue.
     */
    public static PlayRequest toPlayRequest(String uri) {
        MusikUri m = parse(uri);
        if (m == null) return null;
        switch (m.kind) {
            case ISRC:
                return new PlayRequest(null, m.id, "", "", uri, 0);
            case RECORDING:
                return new PlayRequest(null, null, m.recording, m.artist == null ? "" : m.artist, uri, 0);
            default:
                return null;   // works/releases/artists/facets aren't a single playable track
        }
    }

    /** The ISRC named by a {@code musik:isrc:}/legacy content URI, or {@code null}. */
    public static String isrcOf(String uri) {
        MusikUri m = parse(uri);
        return (m != null && m.kind == Kind.ISRC) ? m.id : null;
    }

    // ── Encoding helpers ─────────────────────────────────────────────────────

    /** Percent-encode free text, emitting {@code %20} for space (RFC-0002 §4.1). */
    static String enc(String text) {
        if (text == null) return "";
        return URLEncoder.encode(text, StandardCharsets.UTF_8).replace("+", "%20");
    }

    /** Percent-decode a path segment, keeping a literal {@code +} (not space). */
    static String pctDecode(String seg) {
        if (seg == null) return "";
        return URLDecoder.decode(seg.replace("+", "%2B"), StandardCharsets.UTF_8);
    }

    /** Decode an {@code x-www-form-urlencoded} query value ({@code +} is space). */
    static String formDecode(String v) {
        if (v == null) return "";
        return URLDecoder.decode(v, StandardCharsets.UTF_8);
    }

    /** Lower-case, hyphen-slug a facet label (RFC-0002 §4.2). */
    static String slug(String label) {
        if (label == null) return "";
        String s = label.toLowerCase(Locale.ROOT)
                        .replaceAll("[^a-z0-9]+", "-")
                        .replaceAll("(^-+)|(-+$)", "");
        return enc(s);
    }

    /** Strip separators/whitespace from a standard identifier and upper-case it. */
    static String normalizeId(String id) {
        if (id == null) return "";
        return id.replaceAll("[\\s-]", "").toUpperCase(Locale.ROOT);
    }

    private static int parseTrackNumber(String v) {
        try { return clampTrackNumber(Integer.parseInt(v.trim())); }
        catch (NumberFormatException e) { return 0; }
    }

    private static int clampTrackNumber(int n) {
        return (n < 0 || n > 99) ? 0 : n;
    }
}
