package se.spacify.app;

import java.io.File;
import java.io.IOException;
import java.util.jar.Attributes;
import java.util.jar.JarFile;
import java.util.jar.Manifest;

/**
 * Application identity read from a jar's {@code META-INF/MANIFEST.MF}. Recognised
 * headers:
 * <pre>
 *   Spacify-App-Id:      se.spacify.app.example   (java namespace)
 *   Spacify-App-Class:   se.spacify.app.example.ExampleApplication
 *   Spacify-App-Name:    Example
 *   Spacify-App-Version: 1.0.0
 * </pre>
 */
public final class ApplicationManifest {

    public static final String ID      = "Spacify-App-Id";
    public static final String CLASS   = "Spacify-App-Class";
    public static final String NAME    = "Spacify-App-Name";
    public static final String VERSION = "Spacify-App-Version";

    private final String id;
    private final String name;
    private final String version;
    private final String mainClass;

    private ApplicationManifest(String id, String name, String version, String mainClass) {
        this.id = id;
        this.name = name;
        this.version = version;
        this.mainClass = mainClass;
    }

    /** Parse the main attributes; returns null if this isn't a Spacify plugin. */
    public static ApplicationManifest from(Manifest mf) {
        if (mf == null) return null;
        Attributes a = mf.getMainAttributes();
        String id    = a.getValue(ID);
        String clazz = a.getValue(CLASS);
        if (id == null || id.isBlank() || clazz == null || clazz.isBlank()) return null;
        String name = a.getValue(NAME);
        return new ApplicationManifest(
            id.trim(),
            name != null && !name.isBlank() ? name.trim() : id.trim(),
            orDefault(a.getValue(VERSION), "0.0.0"),
            clazz.trim());
    }

    /** Read the manifest from a plugin jar; returns null if absent/invalid. */
    public static ApplicationManifest fromJar(File jar) throws IOException {
        try (JarFile jf = new JarFile(jar)) {
            return from(jf.getManifest());
        }
    }

    public String getId()        { return id; }
    public String getName()      { return name; }
    public String getVersion()   { return version; }
    public String getMainClass() { return mainClass; }

    private static String orDefault(String v, String def) {
        return v != null && !v.isBlank() ? v.trim() : def;
    }
}
