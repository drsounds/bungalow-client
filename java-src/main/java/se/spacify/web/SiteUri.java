package se.spacify.web;

import java.net.URI;

/**
 * Translates between {@code spacify:site:} navigation URIs and {@code https://}
 * URLs, bidirectionally.
 *
 * <p>Grammar: {@code spacify:site:<host>[:<segment>...][?query][#fragment]}.
 * Colon-separated segments after the host become path components; any query and
 * fragment pass through unchanged. Translation is https-only.
 *
 * <pre>
 *   spacify:site:example.com:foo:bar?x=1  ⇄  https://example.com/foo/bar?x=1
 *   spacify:site:example.com              ⇄  https://example.com
 * </pre>
 */
public final class SiteUri {

    public static final String PREFIX       = "spacify:site:";
    /** Same grammar, used by the Service/store browser (SPServiceWebView). */
    public static final String STORE_PREFIX = "spacify:store:";

    private SiteUri() {}

    public static boolean isSiteUri(String uri) {
        return hasPrefix(uri, PREFIX);
    }

    public static boolean hasPrefix(String uri, String prefix) {
        return uri != null && uri.startsWith(prefix);
    }

    /** The host of a site URI (first segment after the prefix), or null. */
    public static String host(String spacifyUri) {
        return host(spacifyUri, PREFIX);
    }

    /** Prefix-aware host extraction. */
    public static String host(String uri, String prefix) {
        if (!hasPrefix(uri, prefix)) return null;
        String rest = uri.substring(prefix.length());
        int end = rest.length();
        for (int i = 0; i < rest.length(); i++) {
            char c = rest.charAt(i);
            if (c == ':' || c == '?' || c == '#') { end = i; break; }
        }
        String host = rest.substring(0, end);
        return host.isEmpty() ? null : host;
    }

    /** True if the URI points at a host root (no path segments). */
    public static boolean isRoot(String spacifyUri) {
        if (!isSiteUri(spacifyUri)) return false;
        String rest = spacifyUri.substring(PREFIX.length());
        for (int i = 0; i < rest.length(); i++) {
            char c = rest.charAt(i);
            if (c == ':') return false;            // has a path segment
            if (c == '?' || c == '#') return true; // query/fragment only
        }
        return true;
    }

    /** The {@code spacify:site:<host>} root URI for any site URI, or null. */
    public static String rootUri(String spacifyUri) {
        String host = host(spacifyUri);
        return host == null ? null : PREFIX + host;
    }

    /** {@code spacify:site:…} → {@code https://…}, or null if not a site URI. */
    public static String toUrl(String spacifyUri) {
        return toUrl(spacifyUri, PREFIX);
    }

    /** Prefix-aware {@code spacify:<scheme>:…} → {@code https://…}. */
    public static String toUrl(String spacifyUri, String prefix) {
        if (!hasPrefix(spacifyUri, prefix)) return null;
        String rest = spacifyUri.substring(prefix.length());
        if (rest.isEmpty()) return null;

        // Split off the ?query / #fragment suffix (whichever comes first).
        int cut = -1;
        for (int i = 0; i < rest.length(); i++) {
            char c = rest.charAt(i);
            if (c == '?' || c == '#') { cut = i; break; }
        }
        String pathPart = cut < 0 ? rest : rest.substring(0, cut);
        String suffix   = cut < 0 ? ""   : rest.substring(cut);
        if (pathPart.isEmpty()) return null;

        String[] segs = pathPart.split(":");
        if (segs[0].isEmpty()) return null;

        StringBuilder url = new StringBuilder("https://").append(segs[0]);
        for (int i = 1; i < segs.length; i++) {
            if (!segs[i].isEmpty()) url.append('/').append(segs[i]);
        }
        return url.append(suffix).toString();
    }

    /** {@code http(s)://…} → {@code spacify:site:…}, or null if it has no host. */
    public static String toSpacifyUri(String url) {
        return toSpacifyUri(url, PREFIX);
    }

    /** Prefix-aware {@code http(s)://…} → {@code spacify:<scheme>:…}. */
    public static String toSpacifyUri(String url, String prefix) {
        if (url == null) return null;
        try {
            URI u = URI.create(url.trim());
            String host = u.getHost();
            if (host == null) return null;

            StringBuilder sb = new StringBuilder(prefix).append(host);
            String path = u.getRawPath();
            if (path != null && !path.isEmpty()) {
                for (String seg : path.split("/")) {
                    if (!seg.isEmpty()) sb.append(':').append(seg);
                }
            }
            if (u.getRawQuery()    != null) sb.append('?').append(u.getRawQuery());
            if (u.getRawFragment() != null) sb.append('#').append(u.getRawFragment());
            return sb.toString();
        } catch (Exception e) {
            return null;
        }
    }
}
