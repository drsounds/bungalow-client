package se.spacify.app;

import java.nio.file.Files;
import java.nio.file.Path;

/**
 * Central filesystem locations for the plugin system:
 * <ul>
 *   <li>external plugin jars live in {@code ~/Bungalow};</li>
 *   <li>bundled jar plugins live in {@code <app>/plugins};</li>
 *   <li>enabled-state and per-plugin setting values live under
 *       {@code ~/.spacify/plugins} (next to the existing settings.properties).</li>
 * </ul>
 */
public final class ApplicationPaths {

    private ApplicationPaths() {}

    /** External, user-installed plugins. */
    public static Path bungalow() {
        return Path.of(System.getProperty("user.home"), "Bungalow");
    }

    /** Bundled jar plugins shipped beside the application. */
    public static Path appApplicationsDir() {
        try {
            Path code = Path.of(ApplicationPaths.class.getProtectionDomain()
                .getCodeSource().getLocation().toURI());
            Path base = Files.isDirectory(code) ? code : code.getParent();
            return base.resolve("apps");
        } catch (Exception e) {
            return Path.of("plugins");
        }
    }

    /** Directory holding plugin state + per-plugin settings files. */
    public static Path stateDir() {
        return Path.of(System.getProperty("user.home"), ".spacify", "apps");
    }

    /** Enabled/disabled flags keyed by plugin id. */
    public static Path stateFile() {
        return stateDir().resolve("state.properties");
    }

    /** Setting values for a single plugin. */
    public static Path settingsFile(String pluginId) {
        return stateDir().resolve(pluginId + ".properties");
    }
}
