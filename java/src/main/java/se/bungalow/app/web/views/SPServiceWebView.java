package se.bungalow.app.web.views;

import se.bungalow.navigation.ViewStack;
import se.bungalow.web.SiteUri;

/**
 * Embedded browser for music-Service "stores", reached via
 * {@code bungalow:store:<host>[:path]} (same grammar as {@code bungalow:site:},
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
