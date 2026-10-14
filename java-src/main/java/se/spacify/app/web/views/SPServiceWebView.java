package se.spacify.app.web.views;

import se.spacify.navigation.ViewStack;
import se.spacify.web.SiteUri;

/**
 * Embedded browser for music-Service "stores", reached via
 * {@code spacify:store:<host>[:path]} (same grammar as {@code spacify:site:},
 * just a different scheme). It is meant to be shown full-width with the side
 * panels collapsed (see {@code MainWindow}'s immersive handling) and does not
 * offer bookmarking.
 */
public class SPServiceWebView extends SPWebView {

    public SPServiceWebView(ViewStack viewStack) {
        super(viewStack);
    }

    @Override protected String  prefix()           { return SiteUri.STORE_PREFIX; }
    @Override protected boolean supportsBookmarks() { return false; }
    @Override public    String  getName()          { return "Store"; }
}
