package se.spacify.net;

import java.net.URI;
import java.net.URISyntaxException;

public class Uri {
    private URI uri;

    public static Uri parse(String uri) throws URISyntaxException {
        return new Uri(uri);
    }

    public Uri(String _uri) throws URISyntaxException {
        uri = new URI(_uri);
    }

    public URI getUri() {
        return uri;
    }

    public void setUri(URI uri) {
        this.uri = uri;
    }
    public String toString() {
        return uri.toString();
    }
    public String getHost() {
        return uri.getHost();
    }
    public String getScheme() {
        return uri.getScheme();
    }
    public String getPath() {
        return uri.getPath();
    }
}
