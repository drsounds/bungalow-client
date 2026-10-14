package se.spacify.app.youtube;

import java.net.URI;
import java.net.URLEncoder;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.nio.charset.StandardCharsets;
import java.time.Duration;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Resolves a search query to YouTube videos (id + title) — via the Data API when
 * a key is configured, otherwise by scraping the public results page's
 * {@code ytInitialData}. Needed because the IFrame Player API's
 * {@code listType:"search"} loader was removed by YouTube (Nov 2020), and so the
 * "Play with…" chooser can list real candidate videos to pick from.
 */
final class YouTubeSearch {

    private YouTubeSearch() {}

    /** One candidate video: its id and display title. */
    record Result(String videoId, String title) {}

    // videoId immediately followed (within the same videoRenderer) by its title.
    private static final Pattern SCRAPE = Pattern.compile(
        "\"videoId\":\"([A-Za-z0-9_-]{11})\".{0,3000}?\"title\":\\{(?:\"runs\":\\[\\{\"text\":\"|\"simpleText\":\")((?:[^\"\\\\]|\\\\.)*)\"",
        Pattern.DOTALL);

    // A desktop UA + en/US locale + consent cookie so we get the normal results
    // page (with ytInitialData) rather than a consent interstitial.
    private static final String USER_AGENT =
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) "
        + "Chrome/124.0.0.0 Safari/537.36";

    private static final HttpClient CLIENT = HttpClient.newBuilder()
        .followRedirects(HttpClient.Redirect.NORMAL)
        .connectTimeout(Duration.ofSeconds(10))
        .build();

    /** Resolve via the scrape only (no API key). */
    static List<Result> search(String query) {
        return search(query, null);
    }

    /**
     * Top candidate videos for {@code query}. When {@code apiKey} is set, uses the
     * YouTube Data API (more reliable, embeddable-only); otherwise — or if the
     * API call fails (bad key / quota) — falls back to scraping the results page.
     */
    static List<Result> search(String query, String apiKey) {
        if (apiKey != null && !apiKey.isBlank()) {
            List<Result> viaApi = apiSearch(query, apiKey);
            if (!viaApi.isEmpty()) return viaApi;
        }
        return scrapeSearch(query);
    }

    // ── YouTube Data API v3 (when a key is configured) ──────────────────────────────

    private static List<Result> apiSearch(String query, String apiKey) {
        List<Result> results = new ArrayList<>();
        try {
            String url = "https://www.googleapis.com/youtube/v3/search?part=snippet&type=video"
                + "&videoEmbeddable=true&maxResults=12&q="
                + URLEncoder.encode(query, StandardCharsets.UTF_8)
                + "&key=" + URLEncoder.encode(apiKey, StandardCharsets.UTF_8);
            HttpRequest request = HttpRequest.newBuilder(URI.create(url))
                .timeout(Duration.ofSeconds(15)).GET().build();
            String body = CLIENT.send(request, HttpResponse.BodyHandlers.ofString()).body();

            com.google.gson.JsonObject root = com.google.gson.JsonParser.parseString(body).getAsJsonObject();
            if (root.has("items")) {
                for (com.google.gson.JsonElement el : root.getAsJsonArray("items")) {
                    com.google.gson.JsonObject item = el.getAsJsonObject();
                    com.google.gson.JsonObject id = item.getAsJsonObject("id");
                    if (id == null || !id.has("videoId")) continue;
                    String videoId = id.get("videoId").getAsString();
                    String title = videoId;
                    com.google.gson.JsonObject snippet = item.getAsJsonObject("snippet");
                    if (snippet != null && snippet.has("title")) title = snippet.get("title").getAsString();
                    results.add(new Result(videoId, title));
                }
            }
        } catch (Exception ignored) {
            // bad key / quota / network → empty, caller falls back to the scrape.
        }
        return results;
    }

    // ── Public results-page scrape (no API key) ─────────────────────────────────────

    /** Top candidate videos for {@code query} (in result order, de-duplicated). */
    private static List<Result> scrapeSearch(String query) {
        try {
            String url = "https://www.youtube.com/results?hl=en&gl=US&search_query="
                + URLEncoder.encode(query, StandardCharsets.UTF_8);
            HttpRequest request = HttpRequest.newBuilder(URI.create(url))
                .timeout(Duration.ofSeconds(15))
                .header("User-Agent", USER_AGENT)
                .header("Accept-Language", "en-US,en;q=0.9")
                .header("Cookie", "CONSENT=YES+1")
                .GET()
                .build();
            String body = CLIENT.send(request, HttpResponse.BodyHandlers.ofString()).body();

            Matcher m = SCRAPE.matcher(body);
            Map<String, String> byId = new LinkedHashMap<>();   // id → title, de-duped, ordered
            while (m.find() && byId.size() < 12) {
                byId.putIfAbsent(m.group(1), jsonUnescape(m.group(2)));
            }
            List<Result> results = new ArrayList<>();
            byId.forEach((id, title) -> results.add(new Result(id, title)));
            return results;
        } catch (Exception ignored) {
            return List.of();
        }
    }

    /** Decode a JSON string body (\\uXXXX, \\", …) by parsing it as a JSON literal. */
    private static String jsonUnescape(String raw) {
        try { return com.google.gson.JsonParser.parseString("\"" + raw + "\"").getAsString(); }
        catch (Exception e) { return raw; }
    }

    /** Whether {@code s} is already a bare 11-character YouTube video id. */
    static boolean isVideoId(String s) {
        return s != null && s.matches("[A-Za-z0-9_-]{11}");
    }
}
