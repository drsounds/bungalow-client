package se.bungalow.app.web;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;
import se.bungalow.app.Application;
import se.bungalow.app.ApplicationContext;
import se.bungalow.app.web.concept.WebConcept;

/**
 * Built-in plugin providing in-app web browsing. Its contributions — the
 * {@link se.bungalow.app.web.views.SPWebView}/{@code SPServiceWebView}, the "Sites"
 * bookmark sidebar and the {@link se.bungalow.app.web.model.Bookmark} table — are
 * carried by the {@link WebConcept}.
 */
public class WebApplication extends Application {

    @Override
    public void onActivate(ApplicationContext ctx) {
        ctx.registerConcept(new WebConcept(this));
    }

    @Override
    public String getId() {
        return "web";
    }

    @Override
    public String getName() {
        return "Web Browser";
    }

    @Override
    public void onRegister(AspectManager<? extends Aspect> aspectManager) {
        // TODO Auto-generated method stub

    }
}
