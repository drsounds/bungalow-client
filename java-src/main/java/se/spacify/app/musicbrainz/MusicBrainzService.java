package se.spacify.app.musicbrainz;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;
import se.spacify.app.music.model.Artist;
import se.spacify.app.music.model.Recording;
import se.spacify.app.music.model.MusicRelease;
import se.spacify.app.catalogue.service.MusicCatalogueService;

import java.net.URI;
import java.net.URLEncoder;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.nio.charset.StandardCharsets;
import java.time.Duration;
import java.util.ArrayList;
import java.util.List;

/**
 * {@link MusicCatalogueService} backed by the public MusicBrainz web Service
 * (<a href="https://musicbrainz.org/doc/MusicBrainz_API">/ws/2</a>). Searches and
 * MBID/ISRC/ISNI lookups are mapped to transient {@link Release}/{@link Recording}/
 * {@link Artist} instances carrying their identity fields — this is a discovery
 * aspect, so nothing is written to the local library.
 *
 * <p>MusicBrainz requires a descriptive {@code User-Agent} and rate-limits
 * anonymous callers to roughly one request per second; both are honoured here
 * ({@link #throttle()} blocks as needed). All calls perform blocking network I/O
 * and must run off the EDT.
 */
public class MusicBrainzService implements MusicCatalogueService {

    private static final String BASE = "https://musicbrainz.org/ws/2/";
    private static final int    SEARCH_LIMIT = 25;
    /** MusicBrainz asks anonymous clients to stay at/below 1 request per second. */
    private static final long   MIN_REQUEST_INTERVAL_MS = 1000L;

    private final HttpClient http = HttpClient.newBuilder()
        .followRedirects(HttpClient.Redirect.NORMAL)
        .connectTimeout(Duration.ofSeconds(10))
        .build();

    private final String userAgent;

    /** Guards {@link #lastRequestAt} so the throttle serialises concurrent callers. */
    private final Object throttleLock = new Object();
    private long lastRequestAt = 0L;

    /**
     * @param contact a contact URL or email for the {@code User-Agent}; MusicBrainz
     *                wants one so they can reach the operator. May be blank.
     */
    public MusicBrainzService(String contact) {
        String suffix = (contact == null || contact.isBlank()) ? "" : " ( " + contact.trim() + " )";
        this.userAgent = "Spacify/1.0.0" + suffix;
    }

    // ── Service identity ────────────────────────────────────────────────────────

    @Override public String getId()   { return "se.spacify.app.musicbrainz"; }
    @Override public String getName() { return "MusicBrainz"; }

    // ── Search ──────────────────────────────────────────────────────────────────

    @Override
    public List<MusicRelease> searchReleases(String query) {
        List<MusicRelease> out = new ArrayList<>();
        if (isBlank(query)) return out;
        JsonObject root = get("release?query=" + encode(query) + "&limit=" + SEARCH_LIMIT);
        if (root == null) return out;
        for (JsonElement e : array(root, "releases")) {
            out.add(toRelease(e.getAsJsonObject()));
        }
        return out;
    }

    @Override
    public List<Recording> searchRecordings(String query) {
        List<Recording> out = new ArrayList<>();
        if (isBlank(query)) return out;
        JsonObject root = get("recording?query=" + encode(query) + "&limit=" + SEARCH_LIMIT);
        if (root == null) return out;
        for (JsonElement e : array(root, "recordings")) {
            out.add(toRecording(e.getAsJsonObject()));
        }
        return out;
    }

    @Override
    public List<Artist> searchArtists(String query) {
        List<Artist> out = new ArrayList<>();
        if (isBlank(query)) return out;
        JsonObject root = get("artist?query=" + encode(query) + "&limit=" + SEARCH_LIMIT);
        if (root == null) return out;
        for (JsonElement e : array(root, "artists")) {
            out.add(toArtist(e.getAsJsonObject()));
        }
        return out;
    }

    // ── Browsing ────────────────────────────────────────────────────────────────

    @Override
    public List<MusicRelease> browseReleasesByArtist(String artistMbid, int offset, int limit) {
        List<MusicRelease> out = new ArrayList<>();
        if (isBlank(artistMbid)) return out;
        JsonObject root = get("release?artist=" + encode(artistMbid)
            + "&limit=" + clampLimit(limit) + "&offset=" + Math.max(0, offset));
        if (root == null) return out;
        for (JsonElement e : array(root, "releases")) {
            out.add(toRelease(e.getAsJsonObject()));
        }
        return out;
    }

    @Override
    public List<Recording> browseRecordingsByRelease(String releaseMbid, int offset, int limit) {
        List<Recording> out = new ArrayList<>();
        if (isBlank(releaseMbid)) return out;
        JsonObject root = get("recording?release=" + encode(releaseMbid)
            + "&inc=artist-credits"   // browse omits credits by default; needed for the Artists column
            + "&limit=" + clampLimit(limit) + "&offset=" + Math.max(0, offset));
        if (root == null) return out;
        for (JsonElement e : array(root, "recordings")) {
            out.add(toRecording(e.getAsJsonObject()));
        }
        return out;
    }

    /** MusicBrainz caps browse/search page size at 100. */
    private static int clampLimit(int limit) {
        return limit <= 0 ? SEARCH_LIMIT : Math.min(limit, 100);
    }

    // ── Lookups ───────────────────────────────────────────────────────────────

    @Override
    public MusicRelease getReleaseByMbid(String mbid) {
        if (isBlank(mbid)) return null;
        JsonObject root = get("release/" + encode(mbid));
        return root != null ? toRelease(root) : null;
    }

    @Override
    public Recording getRecordingByIsrc(String isrc) {
        if (isBlank(isrc)) return null;
        // The ISRC endpoint returns the recordings carrying that code.
        JsonObject root = get("isrc/" + encode(isrc));
        if (root == null) return null;
        JsonArray recordings = array(root, "recordings");
        return recordings.isEmpty() ? null : toRecording(recordings.get(0).getAsJsonObject());
    }

    @Override
    public Artist getArtistByIsni(String isni) {
        if (isBlank(isni)) return null;
        // No direct ISNI endpoint; an indexed search on the isni field is the
        // documented way to resolve one.
        JsonObject root = get("artist?query=isni:" + encode(isni) + "&limit=1");
        if (root == null) return null;
        JsonArray artists = array(root, "artists");
        return artists.isEmpty() ? null : toArtist(artists.get(0).getAsJsonObject());
    }

    // ── JSON → entity mapping ───────────────────────────────────────────────────

    private static MusicRelease toRelease(JsonObject o) {
        MusicRelease r = new MusicRelease(str(o, "title"));
        r.setMbid(str(o, "id"));
        r.setReleaseDate(str(o, "date"));
        String barcode = str(o, "barcode");
        if (!isBlank(barcode)) r.setUpc(barcode);
        return r;
    }

    private static Recording toRecording(JsonObject o) {
        Recording rec = new Recording(str(o, "title"));
        if (o.has("length") && !o.get("length").isJsonNull()) {
            rec.setDurationMs(o.get("length").getAsLong());
        }
        JsonArray isrcs = o.has("isrcs") && o.get("isrcs").isJsonArray() ? o.getAsJsonArray("isrcs") : new JsonArray();
        if (!isrcs.isEmpty()) rec.setIsrc(isrcs.get(0).getAsString());
        rec.setArtistNames(artistCredit(o));
        return rec;
    }

    /** Join a MusicBrainz {@code artist-credit} array into a display string
     *  (e.g. {@code "A feat. B"}), or null when absent. */
    private static String artistCredit(JsonObject o) {
        StringBuilder sb = new StringBuilder();
        for (JsonElement e : array(o, "artist-credit")) {
            if (!e.isJsonObject()) continue;
            JsonObject c = e.getAsJsonObject();
            String name = str(c, "name");
            if (name != null) sb.append(name);
            String join = str(c, "joinphrase");
            if (join != null) sb.append(join);
        }
        String s = sb.toString().trim();
        return s.isEmpty() ? null : s;
    }

    private static Artist toArtist(JsonObject o) {
        Artist a = new Artist(str(o, "name"));
        a.setMbid(str(o, "id"));
        JsonArray isnis = o.has("isnis") && o.get("isnis").isJsonArray() ? o.getAsJsonArray("isnis") : new JsonArray();
        if (!isnis.isEmpty()) a.setIsni(isnis.get(0).getAsString());
        return a;
    }

    // ── HTTP ──────────────────────────────────────────────────────────────────

    /**
     * Issue a GET against {@code /ws/2/<path>}, forcing {@code fmt=json}, and parse
     * the body. Returns {@code null} on any network, status, or parse failure so
     * callers degrade to empty results.
     */
    private JsonObject get(String path) {
        String url = BASE + path + (path.indexOf('?') >= 0 ? "&" : "?") + "fmt=json";
        try {
            throttle();
            HttpRequest req = HttpRequest.newBuilder(URI.create(url))
                .header("User-Agent", userAgent)
                .header("Accept", "application/json")
                .timeout(Duration.ofSeconds(15))
                .GET()
                .build();
            HttpResponse<String> resp = http.send(req, HttpResponse.BodyHandlers.ofString(StandardCharsets.UTF_8));
            if (resp.statusCode() != 200) return null;
            JsonElement parsed = JsonParser.parseString(resp.body());
            return parsed.isJsonObject() ? parsed.getAsJsonObject() : null;
        } catch (Exception e) {
            return null;
        }
    }

    /** Block until at least {@link #MIN_REQUEST_INTERVAL_MS} has passed since the last request. */
    private void throttle() {
        synchronized (throttleLock) {
            long now = System.currentTimeMillis();
            long wait = MIN_REQUEST_INTERVAL_MS - (now - lastRequestAt);
            if (wait > 0) {
                try { Thread.sleep(wait); } catch (InterruptedException ie) { Thread.currentThread().interrupt(); }
            }
            lastRequestAt = System.currentTimeMillis();
        }
    }

    // ── JSON helpers ────────────────────────────────────────────────────────────

    private static JsonArray array(JsonObject o, String key) {
        return o.has(key) && o.get(key).isJsonArray() ? o.getAsJsonArray(key) : new JsonArray();
    }

    private static String str(JsonObject o, String key) {
        return o.has(key) && !o.get(key).isJsonNull() ? o.get(key).getAsString() : null;
    }

    private static String encode(String s) {
        return URLEncoder.encode(s, StandardCharsets.UTF_8);
    }

    private static boolean isBlank(String s) {
        return s == null || s.isBlank();
    }

    @Override
    public void onRegister(AspectManager<? extends Aspect> aspectManager) {
        // TODO Auto-generated method stub
        
    }
}
