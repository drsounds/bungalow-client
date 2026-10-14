package se.spacify.web;

import me.friwi.jcefmaven.CefAppBuilder;
import org.cef.CefApp;
import org.cef.CefClient;
import org.cef.callback.CefSchemeHandlerFactory;

import java.io.File;

/**
 * Owns the process-wide {@link CefApp}. The first call to {@link #newClient()}
 * builds it via jcefmaven, which downloads and unpacks the matching native
 * Chromium bundle into {@code ~/.spacify/jcef} (cached for later runs). This
 * call is slow and must run off the EDT. {@link #dispose()} shuts CEF down.
 */
public final class CefRuntime {

    private static CefApp app;

    private CefRuntime() {}

    /** Build CEF if needed and return a fresh client. Heavy on first call. */
    public static synchronized CefClient newClient() throws Exception {
        return app().createClient();
    }

    /**
     * Register a scheme/domain handler factory on the process-wide app (building
     * CEF first if needed). Used to serve in-app pages from a real {@code https}
     * origin so embedded web players (e.g. the YouTube IFrame API) treat them as a
     * secure, non-null origin rather than {@code file://}.
     */
    public static synchronized boolean registerSchemeHandlerFactory(
            String scheme, String domain, CefSchemeHandlerFactory factory) throws Exception {
        return app().registerSchemeHandlerFactory(scheme, domain, factory);
    }

    private static CefApp app() throws Exception {
        if (app == null) {
            CefAppBuilder builder = new CefAppBuilder();
            builder.setInstallDir(new File(System.getProperty("user.home"), ".spacify/jcef"));
            builder.getCefSettings().windowless_rendering_enabled = false;
            app = builder.build();
        }
        return app;
    }

    public static synchronized void dispose() {
        if (app != null) {
            app.dispose();
            app = null;
        }
    }
}
