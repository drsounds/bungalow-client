package se.spacify.app.testapp.controller;

import java.io.IOException;
import java.io.InputStream;
import java.io.UncheckedIOException;
import java.nio.charset.StandardCharsets;
import java.util.Map;

import se.spacify.app.spider.Request;
import se.spacify.app.spider.controller.Controller;
import se.spacify.net.Uri;

/**
 * The {@code spacify:testapp} controller: a hello-world Spider handler whose template
 * is the {@code views/test.xml} resource shipped alongside this app. It renders an
 * "Overview" page with the current date, a 1..10 loop and a refresh button.
 */
public class TestController extends Controller {

    /** Classpath location of the bundled template (copied from {@code src/main/java} by the build). */
    private static final String TEMPLATE = "/se/spacify/app/testapp/views/test.xml";

    @Override
    public boolean acceptsUri(Uri uri) {
        return uri != null
            && "spacify".equals(uri.getScheme())
            && uri.toString().startsWith("spacify:testapp");
    }

    @Override
    protected String template(Request request) {
        try (InputStream in = TestController.class.getResourceAsStream(TEMPLATE)) {
            if (in == null) {
                throw new IllegalStateException("Missing Spider template resource: " + TEMPLATE);
            }
            return new String(in.readAllBytes(), StandardCharsets.UTF_8);
        } catch (IOException e) {
            throw new UncheckedIOException("Failed to read Spider template: " + TEMPLATE, e);
        }
    }

    /** This hello-world app renders purely from Lua/request globals, so it exposes no model. */
    @Override
    protected Map<String, Object> data(Request request) {
        return Map.of();
    }
}
