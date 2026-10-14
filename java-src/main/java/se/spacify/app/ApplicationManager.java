package se.spacify.app;

import se.spacify.design.Design;
import se.spacify.feature.Feature;
import se.spacify.navigation.View;
import se.spacify.navigation.SidebarNode;
import se.spacify.service.Service;
import se.spacify.skinning.Skin;

import se.spacify.ui.LeftLibraryMenu;
import se.spacify.ui.MainWindow;
import se.spacify.ui.theme.Theme;
import se.spacify.ui.chrome.Chrome;
import se.spacify.concept.Concept;
import se.spacify.concept.ConceptContext;
import se.spacify.navigation.ViewStack;

import javax.swing.Icon;
import javax.swing.tree.DefaultMutableTreeNode;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URLClassLoader;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Properties;

/**
 * Singleton registry that discovers plugins, instantiates their main classes,
 * and activates/deactivates them on demand. Activation wires a plugin's
 * contributions (Services, features, views, leftLibraryMenu nodes) into the running app
 * through a recording {@link ApplicationContext}; deactivation replays those records
 * in reverse so a disabled or removed plugin leaves no trace.
 *
 * <p>Enabled/disabled flags persist to {@code ~/.spacify/plugins/state.properties};
 * per-plugin setting values persist via {@link ApplicationSettings}.
 */
public final class ApplicationManager {
 

    private MainWindow mainWindow;

    public MainWindow getMainWindow() {
        return mainWindow;
    }

    public void setMainWindow(MainWindow mainWindow) {
        this.mainWindow = mainWindow;
    }

    public ApplicationManager(MainWindow mainWindow) {
        this.mainWindow = mainWindow; // avoid "unused" warning; the manager is wired to the UI in MainWindow.init()
    }

    private final ApplicationLoader loader = new ApplicationLoader();
    private final Map<String, ManagedApplication> plugins = new LinkedHashMap<>();
    private final Properties state = new Properties();
    private final List<Runnable> listeners = new ArrayList<>();

    private ViewStack viewStack;
    private LeftLibraryMenu     leftLibraryMenu;

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /** Wire the manager to the live UI; call once before {@link #start()}. */
    public void init(ViewStack viewStack, LeftLibraryMenu leftLibraryMenu) {
        this.viewStack = viewStack;
        this.leftLibraryMenu = leftLibraryMenu;
        loadState();
    }

    /** Discover all plugins and activate the enabled ones. */
    public void start() {
        discoverAll();
        for (ManagedApplication m : plugins.values()) {
            if (m.enabled && !m.isActive()) activate(m);
        }
        notifyListeners();
    }

    /** (Re)scan the three sources; new descriptors are instantiated but not yet activated. */
    public void discoverAll() {
        for (ApplicationDescriptor d : loader.discover()) {
            if (plugins.containsKey(d.getId())) continue;
            ManagedApplication m = instantiate(d);
            if (m != null) plugins.put(d.getId(), m);
        }
    }

    // ── Enable / disable ───────────────────────────────────────────────────────

    public void setEnabled(String id, boolean enable) {
        ManagedApplication m = plugins.get(id);
        if (m == null || m.enabled == enable) return;
        if (enable && !m.isActive())      activate(m);
        else if (!enable && m.isActive()) deactivate(m);
        m.enabled = enable;
        state.setProperty(id, String.valueOf(enable));
        saveState();
        notifyListeners();
    }

    // ── Install / uninstall ─────────────────────────────────────────────────────

    /** Copy an external jar into {@code ~/Bungalow}, then load, enable, and activate it. */
    public boolean install(File jar) {
        try {
            Files.createDirectories(ApplicationPaths.bungalow());
            Path dest = ApplicationPaths.bungalow().resolve(jar.getName());
            Files.copy(jar.toPath(), dest, StandardCopyOption.REPLACE_EXISTING);

            ApplicationDescriptor d = loader.fromJar(dest.toFile(), ApplicationDescriptor.Source.EXTERNAL);
            if (d == null) { Files.deleteIfExists(dest); return false; }

            // Replace any existing plugin with the same id (upgrade).
            ManagedApplication existing = plugins.remove(d.getId());
            if (existing != null) { if (existing.isActive()) deactivate(existing); closeLoader(existing.descriptor); }

            ManagedApplication m = instantiate(d);
            if (m == null) { Files.deleteIfExists(dest); return false; }
            plugins.put(d.getId(), m);
            m.enabled = true;
            state.setProperty(d.getId(), "true");
            activate(m);
            saveState();
            notifyListeners();
            return true;
        } catch (IOException e) {
            return false;
        }
    }

    /** Deactivate and delete an external plugin. Built-ins can only be disabled. */
    public boolean uninstall(String id) {
        ManagedApplication m = plugins.get(id);
        if (m == null || !m.descriptor.isRemovable()) return false;
        if (m.isActive()) deactivate(m);
        closeLoader(m.descriptor);
        plugins.remove(id);
        state.remove(id);
        try { if (m.descriptor.getJar() != null) Files.deleteIfExists(m.descriptor.getJar().toPath()); }
        catch (IOException ignored) {}
        saveState();
        notifyListeners();
        return true;
    }

    // ── Queries / listeners ─────────────────────────────────────────────────────

    public List<ManagedApplication> getApplications() {
        return Collections.unmodifiableList(new ArrayList<>(plugins.values()));
    }

    public void addChangeListener(Runnable r) { listeners.add(r); }

    private void notifyListeners() { for (Runnable r : List.copyOf(listeners)) r.run(); }

    // ── Activation internals ────────────────────────────────────────────────────

    private ManagedApplication instantiate(ApplicationDescriptor d) {
        try {
            Class<?> c = Class.forName(d.getMainClass(), true, d.getClassLoader());
            Application p = (Application) c.getDeclaredConstructor().newInstance();
            // Inject the manager so the plugin can reach the live app (MainWindow,
            // ServiceManager, …) from onActivate and beyond.
            p.manager = this;
            return new ManagedApplication(d, p);
        } catch (Throwable e) {
            System.err.println("Failed to load plugin " + d.getId() + ": " + e);
            return null;
        }
    }

    private void activate(ManagedApplication m) {
        Recorder rec = new Recorder(m);
        try {
            m.plugin.onActivate(rec);
            m.recorder = rec;
        } catch (Throwable e) {
            System.err.println("Failed to activate plugin " + m.descriptor.getId() + ": " + e);
            rec.undo();
        }
    }

    private void deactivate(ManagedApplication m) {
        try { m.plugin.onDeactivate(); }
        catch (Throwable e) { System.err.println("Error deactivating " + m.descriptor.getId() + ": " + e); }
        if (m.recorder != null) { m.recorder.undo(); m.recorder = null; }
    }

    private static void closeLoader(ApplicationDescriptor d) {
        if (d.getClassLoader() instanceof URLClassLoader u) {
            try { u.close(); } catch (IOException ignored) {}
        }
    }

    // ── State persistence ───────────────────────────────────────────────────────

    private void loadState() {
        if (!Files.exists(ApplicationPaths.stateFile())) return;
        try (InputStream in = Files.newInputStream(ApplicationPaths.stateFile())) {
            state.load(in);
        } catch (IOException ignored) {}
    }

    private void saveState() {
        try {
            Files.createDirectories(ApplicationPaths.stateDir());
            try (OutputStream out = Files.newOutputStream(ApplicationPaths.stateFile())) {
                state.store(out, "Spacify plugin enabled-state");
            }
        } catch (IOException ignored) {}
    }

    private boolean enabledByDefault(String id) {
        return Boolean.parseBoolean(state.getProperty(id, "true"));
    }

    // ── Managed plugin record ───────────────────────────────────────────────────

    /** Public view over a discovered plugin, used by the manager UI. */
    public final class ManagedApplication {
        private final ApplicationDescriptor descriptor;
        private final Application           plugin;
        private final ApplicationSettings   settings;
        private boolean                enabled;
        private boolean                active;
        private Recorder               recorder;
        private Icon                   icon;
        public Icon getIcon() { return icon; }
        public void setIcon(Icon icon) { this.icon = icon; }
        
        ManagedApplication(ApplicationDescriptor descriptor, Application plugin) {
            this.descriptor = descriptor;
            this.plugin = plugin;
            this.settings = new ApplicationSettings(descriptor.getId(), plugin.getSettingsSchema());
            this.enabled = enabledByDefault(descriptor.getId());
            this.active = false;
        }

        public ApplicationDescriptor    getDescriptor() { return descriptor; }
        public boolean             isEnabled()     { return enabled; }
        public boolean             isActive()      { return active; } 
        public List<ApplicationSetting> getSchema()     { return plugin.getSettingsSchema(); }
        public ApplicationSettings      getSettings()   { return settings; }
    }

    // ── Recording context: wires contributions and can undo them ────────────────

    private final class Recorder implements ApplicationContext, ConceptContext {
        private final ManagedApplication owner;
        private final List<Service>                services = new ArrayList<>();
        private final List<Feature>                features = new ArrayList<>();
        private final List<View>                 views    = new ArrayList<>();
        private final List<DefaultMutableTreeNode> nodes    = new ArrayList<>();
        private final List<Chrome> 				   chromes = new ArrayList<>();
        private final List<Skin> 				   skins = new ArrayList<>();
        private final List<Design> 				   designs = new ArrayList<>();
        private final List<Theme> 				   themes = new ArrayList<>();
        private final List<Concept>                concepts = new ArrayList<>();
        private final List<se.spacify.search.SearchProvider> searchProviders = new ArrayList<>();

        Recorder(ManagedApplication owner) { this.owner = owner; }

        @Override public String pluginId() { return owner.descriptor.getId(); }

        @Override public <T> com.j256.ormlite.dao.Dao<T, Integer> registerEntity(Class<T> entity) {
            return se.spacify.db.DatabaseManager.getInstance().registerEntity(entity);
        }

        @Override public ApplicationSettings settings() { return owner.settings; }

        @Override public ViewStack viewStack() { return viewStack; }

        @Override public void registerService(Service s) {
            getMainWindow().getServiceManager().register(s);
            s.onStart();
            services.add(s);
        }
		@Override
		public void registerChrome(Chrome c) {
            getMainWindow().getChromeManager().register(c);
			// TODO Auto-generated method stub
            chromes.add(c);
		}

		@Override
		public void registerDesign(Design c) {
            getMainWindow().getDesignManager().register(c);
			// TODO Auto-generated method stub
            designs.add(c);
		}
		@Override
		public void registerConcept(Concept c) {
            getMainWindow().getConceptManager().register(c);
            concepts.add(c);
            // Activate the concept through this same recorder, so the views,
            // services and sidebar nodes it contributes are recorded against the
            // owning plugin and torn down with it on undo().
            c.onActivate(this);
		}
        @Override public void registerView(View v) {
            if (viewStack != null) viewStack.registerView(v);
            views.add(v);
        }

        @Override public void registerSearchProvider(se.spacify.search.SearchProvider p) {
            getMainWindow().getSearchManager().register(p);
            searchProviders.add(p);
        }

        @Override public SidebarHandle addSidebarNode(SidebarNode n) {
            if (leftLibraryMenu == null) return NoopSidebarHandle.INSTANCE;
            DefaultMutableTreeNode node = leftLibraryMenu.addSidebarNode(n);
            nodes.add(node);
            return new SidebarHandleImpl(leftLibraryMenu, node);
        }

        @Override public void registerFeature(Feature f) {
            getMainWindow().getFeatureManager().register(f);
            features.add(f);
            f.getViews().forEach(this::registerView);
            f.getSidebarNodes().forEach(this::addSidebarNode);
        }

        @Override public void registerSkin(Skin s) {
            getMainWindow().getSkinManager().register(s);
            skins.add(s);
        }

        /** Replay registrations in reverse, removing every contribution. */
        void undo() {
            // Let concepts release their own resources first (e.g. event listeners),
            // then drop the views/services/nodes they (and the plugin) registered.
            for (Concept c : concepts) { c.onDeactivate(); getMainWindow().getConceptManager().unregister(c); }
            for (se.spacify.search.SearchProvider p : searchProviders) getMainWindow().getSearchManager().unregister(p);
            for (DefaultMutableTreeNode n : nodes) if (leftLibraryMenu != null) leftLibraryMenu.removeSidebarNode(n);
            for (View v : views) if (viewStack != null) viewStack.unregisterView(v);
            for (Service s : services) getMainWindow().getServiceManager().unregister(s);
             for (Skin s : skins) getMainWindow().getSkinManager().unregister(s);
            for (Feature f : features) getMainWindow().getFeatureManager().unregister(f);
            nodes.clear(); views.clear(); services.clear(); features.clear(); chromes.clear(); skins.clear(); concepts.clear(); searchProviders.clear();
        }

        @Override
        public void registerTheme(Theme t) {
            // TODO Auto-generated method stub
            getMainWindow().getThemeManager().register(t);
            themes.add(t);
        }
    }
}
