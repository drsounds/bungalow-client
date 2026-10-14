package se.spacify.app.web;

import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;
import se.spacify.app.Application;
import se.spacify.app.ApplicationContext;
import se.spacify.app.web.concept.WebConcept;

/**
 * Built-in plugin providing in-app web browsing. Its contributions — the
 * {@link se.spacify.app.web.views.SPWebView}/{@code SPServiceWebView}, the "Sites"
 * bookmark sidebar and the {@link se.spacify.app.web.model.Bookmark} table — are
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
