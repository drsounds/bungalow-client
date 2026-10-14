package se.spacify.app;

import java.io.File;

/**
 * Everything needed to instantiate and identify a plugin, regardless of where
 * it came from: built into the main bundle, a bundled jar in {@code <app>/plugins},
 * or an external jar in {@code ~/Bungalow}. Each jar-backed descriptor owns its
 * own {@link ClassLoader} for isolation.
 */
public final class ApplicationDescriptor {

    public enum Source { BUILTIN_BUNDLE, APP_DIR, EXTERNAL }

    private final String      id;
    private final String      name;
    private final String      version;
    private final String      mainClass;
    private final Source       source;
    private final File         jar;        // null for BUILTIN_BUNDLE
    private final ClassLoader  classLoader;

    public ApplicationDescriptor(String id, String name, String version, String mainClass,
                            Source source, File jar, ClassLoader classLoader) {
        this.id = id;
        this.name = name;
        this.version = version;
        this.mainClass = mainClass;
        this.source = source;
        this.jar = jar;
        this.classLoader = classLoader;
    }

    public String      getId()          { return id; }
    public String      getName()        { return name; }
    public String      getVersion()     { return version; }
    public String      getMainClass()   { return mainClass; }
    public Source      getSource()      { return source; }
    public File        getJar()         { return jar; }
    public ClassLoader getClassLoader() { return classLoader; }

    /** Only externally-installed plugins can be uninstalled (built-ins disable only). */
    public boolean isRemovable() { return source == Source.EXTERNAL; }
}
