package se.bungalow.app.testapp;

import se.bungalow.app.Application;
import se.bungalow.app.ApplicationContext;
import se.bungalow.app.testapp.views.TestAppView;
import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;
import se.bungalow.navigation.SidebarNode;

/**
 * Hello-world built-in plugin demonstrating the Spider template system: it registers
 * a {@link TestAppView} for {@code bungalow:testapp} and a sidebar node that navigates
 * to it. The view's tabs are rendered from {@code views/test.xml}.
 */
public class TestApplication extends Application {

    @Override
    public void onActivate(ApplicationContext ctx) {
        ctx.registerView(new TestAppView(ctx.viewStack()));
        ctx.addSidebarNode(new SidebarNode("Test App", "bungalow:testapp"));
    }

    @Override
    public String getId() {
        return "testapp";
    }

    @Override
    public String getName() {
        return "Test App";
    }

    @Override
    public void onRegister(AspectManager<? extends Aspect> aspectManager) {
    }
}
