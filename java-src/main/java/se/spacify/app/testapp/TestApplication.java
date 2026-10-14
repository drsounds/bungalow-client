package se.spacify.app.testapp;

import se.spacify.app.Application;
import se.spacify.app.ApplicationContext;
import se.spacify.app.testapp.views.TestAppView;
import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;
import se.spacify.navigation.SidebarNode;

/**
 * Hello-world built-in plugin demonstrating the Spider template system: it registers
 * a {@link TestAppView} for {@code spacify:testapp} and a sidebar node that navigates
 * to it. The view's tabs are rendered from {@code views/test.xml}.
 */
public class TestApplication extends Application {

    @Override
    public void onActivate(ApplicationContext ctx) {
        ctx.registerView(new TestAppView(ctx.viewStack()));
        ctx.addSidebarNode(new SidebarNode("Test App", "spacify:testapp"));
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
