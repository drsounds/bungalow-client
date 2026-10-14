package se.spacify.app;

import javax.swing.Icon;

import se.spacify.aspect.Aspect;
import se.spacify.navigation.ViewStack;

import java.util.List;

/**
 * Entry point a plugin's main class implements. Identity (id, name, version) is
 * carried by the {@link ApplicationManifest} (jar {@code META-INF/MANIFEST.MF}
 * headers, or {@link BuiltinApplicationRegistry} for main-bundle plugins); the class
 * here supplies the display icon, an optional typed settings schema, and the
 * registration logic.
 *
 * <p>A plugin may contribute any mix of {@link se.spacify.service.Service}s,
 * {@link se.spacify.feature.Feature}s, {@link se.spacify.navigation.View}s
 * (each accepting a {@code spacify:} URI) and sidebar nodes — all wired through
 * the {@link ApplicationContext} so they can be cleanly removed again on disable.
 */
public abstract class Application implements Aspect {
    ApplicationManager manager;
    public ApplicationManager getManager() {
        return manager;
    }
    public Application() {
        super();
    }
    public ViewStack getViewStack() {
        return manager.getMainWindow().getViewStack();
    }
    /** Register Services, features, views and sidebar nodes via {@code ctx}. */
    public void onActivate(ApplicationContext ctx) {}

    /** Release any resources acquired in {@link #onActivate}. */
    public void onDeactivate() {}

    /** Icon shown in the plugin manager; null falls back to a default. */
    public Icon getIcon() { return null; }

    /** Typed settings this plugin exposes; the manager renders an editor per entry. */
    public List<ApplicationSetting> getSettingsSchema() { return List.of(); }
}
