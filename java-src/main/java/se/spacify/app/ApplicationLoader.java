package se.spacify.app;

import java.io.File;
import java.net.URL;
import java.net.URLClassLoader;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Stream;

/**
 * Discovers {@link ApplicationDescriptor}s from the three sources (built-in bundle,
 * {@code <app>/plugins}, {@code ~/Bungalow}) and builds an isolated
 * {@link URLClassLoader} (parent = application loader) for each jar plugin.
 */
final class ApplicationLoader {

    private final ClassLoader parent = ApplicationLoader.class.getClassLoader();

    /** All discoverable descriptors, built-ins first. */
    List<ApplicationDescriptor> discover() {
        List<ApplicationDescriptor> out = new ArrayList<>(BuiltinApplicationRegistry.descriptors());
        out.addAll(scanDir(ApplicationPaths.appApplicationsDir(), ApplicationDescriptor.Source.APP_DIR));
        out.addAll(scanDir(ApplicationPaths.bungalow(),      ApplicationDescriptor.Source.EXTERNAL));
        return out;
    }

    private List<ApplicationDescriptor> scanDir(Path dir, ApplicationDescriptor.Source source) {
        List<ApplicationDescriptor> out = new ArrayList<>();
        if (dir == null || !Files.isDirectory(dir)) return out;
        try (Stream<Path> jars = Files.list(dir)) {
            jars.filter(p -> p.toString().toLowerCase().endsWith(".jar"))
                .sorted()
                .forEach(p -> {
                    ApplicationDescriptor d = fromJar(p.toFile(), source);
                    if (d != null) out.add(d);
                });
        } catch (Exception ignored) {}
        return out;
    }

    /** Build a descriptor for a single jar, or null if it isn't a valid plugin. */
    ApplicationDescriptor fromJar(File jar, ApplicationDescriptor.Source source) {
        try {
            ApplicationManifest mf = ApplicationManifest.fromJar(jar);
            if (mf == null) return null;
            URLClassLoader cl = new URLClassLoader(
                new URL[]{ jar.toURI().toURL() }, parent);
            return new ApplicationDescriptor(mf.getId(), mf.getName(), mf.getVersion(),
                mf.getMainClass(), source, jar, cl);
        } catch (Exception e) {
            return null;
        }
    }
}
