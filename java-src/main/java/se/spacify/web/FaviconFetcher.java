package se.spacify.web;

import javax.imageio.ImageIO;
import java.awt.Image;
import java.awt.image.BufferedImage;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Best-effort favicon retrieval for a host. Prefers {@code <link rel="icon">}
 * declarations in the page (usually PNG) and falls back to {@code /favicon.ico}.
 * Whatever decodes is normalised to 16x16 PNG bytes; ICO-only icons that Java's
 * ImageIO can't read yield null (a default icon is shown instead).
 *
 * <p>Performs blocking network I/O — call off the EDT.
 */
public final class FaviconFetcher {

    private static final int SIZE = 16;
    private static final Pattern LINK_ICON = Pattern.compile(
        "<link\\b[^>]*\\brel=[\"']?[^\"'>]*icon[^\"'>]*[\"']?[^>]*>",
        Pattern.CASE_INSENSITIVE);
    private static final Pattern HREF = Pattern.compile(
        "href=[\"']([^\"']+)[\"']", Pattern.CASE_INSENSITIVE);

    private static final HttpClient HTTP = HttpClient.newBuilder()
        .followRedirects(HttpClient.Redirect.NORMAL)
        .connectTimeout(Duration.ofSeconds(5))
        .build();

    private FaviconFetcher() {}

    /** @return 16x16 PNG bytes, or null if no usable favicon was found. */
    public static byte[] fetch(String host) {
        if (host == null || host.isBlank()) return null;
        String base = "https://" + host;
        for (String candidate : iconCandidates(base)) {
            byte[] png = tryIcon(candidate);
            if (png != null) return png;
        }
        return null;
    }

    private static List<String> iconCandidates(String base) {
        List<String> urls = new ArrayList<>();
        try {
            String html = get(base + "/");
            if (html != null) {
                Matcher link = LINK_ICON.matcher(html);
                while (link.find()) {
                    Matcher href = HREF.matcher(link.group());
                    if (href.find()) {
                        try { urls.add(URI.create(base + "/").resolve(href.group(1)).toString()); }
                        catch (Exception ignored) {}
                    }
                }
            }
        } catch (Exception ignored) {}
        urls.add(base + "/favicon.ico");   // always try the well-known location last
        return urls;
    }

    private static byte[] tryIcon(String url) {
        try {
            byte[] raw = getBytes(url);
            if (raw == null) return null;
            BufferedImage img = ImageIO.read(new ByteArrayInputStream(raw));
            if (img == null) return null;   // undecodable (e.g. ICO/SVG)
            BufferedImage scaled = new BufferedImage(SIZE, SIZE, BufferedImage.TYPE_INT_ARGB);
            var g = scaled.createGraphics();
            g.drawImage(img.getScaledInstance(SIZE, SIZE, Image.SCALE_SMOOTH), 0, 0, null);
            g.dispose();
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            ImageIO.write(scaled, "png", out);
            return out.toByteArray();
        } catch (Exception e) {
            return null;
        }
    }

    private static String get(String url) throws Exception {
        byte[] b = getBytes(url);
        return b == null ? null : new String(b, java.nio.charset.StandardCharsets.UTF_8);
    }

    private static byte[] getBytes(String url) {
        try {
            HttpRequest req = HttpRequest.newBuilder(URI.create(url))
                .timeout(Duration.ofSeconds(5))
                .header("User-Agent", "Spacify")
                .GET().build();
            HttpResponse<byte[]> resp = HTTP.send(req, HttpResponse.BodyHandlers.ofByteArray());
            return resp.statusCode() == 200 ? resp.body() : null;
        } catch (Exception e) {
            return null;
        }
    }
}
