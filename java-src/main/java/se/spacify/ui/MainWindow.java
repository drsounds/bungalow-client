package se.spacify.ui;

import se.spacify.controls.SplitPane;
import se.spacify.design.Design;
import se.spacify.design.DesignManager;
import se.spacify.feature.FeatureManager;
import se.spacify.navigation.ViewStack;
import se.spacify.app.ApplicationManager;
import se.spacify.app.media.service.MediaService;
import se.spacify.app.media.views.NowPlayingView;
import se.spacify.service.ServiceManager;
import se.spacify.service.media.PlaybackCoordinator;
import se.spacify.service.media.PlayQueue;
import se.spacify.concept.ConceptManager;
import se.spacify.config.ConfigManager;
import se.spacify.controls.Panel;
import se.spacify.skinning.Skin;
import se.spacify.skinning.SkinManager;
import se.spacify.ui.chrome.Chrome;
import se.spacify.ui.chrome.ChromeManager;
import se.spacify.ui.theme.Taste;
import se.spacify.ui.theme.Theme;
import se.spacify.ui.theme.ThemeManager;

import javax.swing.*;
import java.awt.*;

public class MainWindow extends JFrame {
    private static final long serialVersionUID = 2144395787232553079L;

    /** The live window, so panels can resolve theme/skin/taste before they are attached. */
    private static MainWindow instance;
    public static MainWindow getInstance() { return instance; }

    private Taste taste;
    
    private LeftLibraryMenu leftLibraryMenu;
    public void setLeftLibraryMenu(LeftLibraryMenu leftLibraryMenu) {
        this.leftLibraryMenu = leftLibraryMenu;
    }

    private Chrome chrome; 

    private ViewStack viewStack;
    public ViewStack getViewStack() { return viewStack; }
    private NowPlayingView nowPlayingView;
    public NowPlayingView getNowPlayingView() { return nowPlayingView; }
    /** The footer is built and owned by the active chrome. */
    public AppFooter getAppFooter() { return getChrome() != null ? getChrome().getAppFooter() : null; }
    public LeftLibraryMenu getLeftLibraryMenu() { return leftLibraryMenu; }
    public void setViewStack(ViewStack viewStack) {
        this.viewStack = viewStack;
    }

    private ConceptManager conceptManager;
    public ConceptManager getConceptManager() {
        return conceptManager;
    }

    private ThemeManager themeManager;
    public ThemeManager getThemeManager() {
        return themeManager;
    }

    private ChromeManager chromeManager;
    public ChromeManager getChromeManager() {
        return chromeManager;
    }
    private DesignManager designManager;
    public DesignManager getDesignManager() {
        return designManager;
    }
    private FeatureManager featureManager;
    public FeatureManager getFeatureManager() {
        return featureManager;
    }

    private ServiceManager serviceManager;
    public ServiceManager getServiceManager() {
        return serviceManager;
    }
    private se.spacify.search.SearchManager searchManager;
    public se.spacify.search.SearchManager getSearchManager() {
        return searchManager;
    }
    private ApplicationManager pluginManager;
    public ApplicationManager getApplicationManager() {
        return pluginManager;
    }

    public void setApplicationManager(ApplicationManager pluginManager) {
        this.pluginManager = pluginManager;
    }
    private SkinManager skinManager;

    public SkinManager getSkinManager() {
        return skinManager;
    }

    public void setSkinManager(SkinManager skinManager) {
        this.skinManager = skinManager;
    }

    public Taste getTaste() {
        return taste;
    }

    public void setTaste(Taste taste) {
        if (getChrome() != null) {
            remove(getChrome().getComponent());
        }
        this.taste = taste;
        if (getChrome() != null) {
            add(getChrome().getComponent(), BorderLayout.CENTER);
        }
    }

    public Theme getTheme() {
        return getTaste().getTheme();
    }
    
    public Design getDesign() {
        return getTaste().getDesign();
    }
    public void setDesign(Design design) {
        if (design == null) return;
        getTaste().setDesign(design);
        setSkin(design.getSkin());

        Chrome next = design.getChrome();
        next.setViewStack(viewStack);
        // Hand the Chrome the shared sidebar so the plugin-contributed nodes show.
        next.setLeftLibraryMenu(leftLibraryMenu);
        installChrome(next);
    }

    /** Build the Chrome's UI tree and swap it into the window's CENTER. */
    private void installChrome(Chrome next) {
        if (chrome != null) remove(chrome.getComponent());
        chrome = next;
        chrome.build();
        add(chrome.getComponent(), BorderLayout.CENTER);
        revalidate();
        repaint();
    }
    public Skin getSkin() {
        return getTaste().getSkin();
    }

    public void setSkin(Skin skin) {
        this.getTaste().setSkin(skin);
    }
    private boolean userWantsSidebar = true;  // user's manual show/hide preference
    private boolean immersive = false;        // full-width store browsing

    private ConfigManager config;
    public Chrome getChrome() {
       return chrome;
    }
	public Panel getAppPanel() {
		return getChrome().getAppPanel();
	} 

	public LeftMenuPanel getLeftMenuPanel() {
		return getChrome().getLeftMenuPanel();
	}

	public Panel getCenterPanel() {
		return getChrome().getCenterPanel();
	}
 
	public TopBar getTopBar() {
		return getChrome().getTopBar();
	} 
	public SplitPane getLeftSplit() {
		return getChrome().getLeftSplit();
	}
	public SplitPane getMainSplit() {
		return getChrome().getMainSplit();
	}
	public AppHeader getAppHeader() {
		return getChrome().getAppHeader();
	} 
    public void setTheme(Theme theme) { 
        getTaste().setTheme(theme);
        rebuildTheme();
    }
    public MainWindow() {
        super("Spacify");
        // Expose this window before anything else so panels constructed during
        // start-up (and not yet attached) can still resolve theme/skin/taste.
        instance = this;

        // 1. Core services that plugins register *into* must exist BEFORE any plugin
        //    activates — otherwise a plugin's onActivate would register into nulls,
        //    which is the start-up cycle (plugin → ServiceManager/SkinManager/…).
        viewStack = new ViewStack();
        taste = new Taste();
        themeManager   = new ThemeManager(this);
        serviceManager = new ServiceManager(this);
        conceptManager = new ConceptManager(this);
        chromeManager  = new ChromeManager(this);
        designManager  = new DesignManager(this);
        featureManager = new FeatureManager(this);
        skinManager    = new SkinManager(this);
        searchManager  = new se.spacify.search.SearchManager(this);

        // 2. The sidebar is owned here (not by the Chrome) and created before plugins
        //    so plugin-contributed nodes land in the menu the Chrome later embeds.
        leftLibraryMenu = new LeftLibraryMenu(viewStack);

        // 3. Now activate plugins: they register Services, Designs, Chromes, Skins,
        //    Themes and sidebar nodes into the managers created above.
        pluginManager = new ApplicationManager(this);
        pluginManager.init(getViewStack(), getLeftLibraryMenu());
        pluginManager.start();

        // 4. Apply persisted taste (theme/accent), now that themes are registered.
        config = new ConfigManager(this);
        config.load();

        serviceManager.startAll();

        // Wire the playback coordinator to this window so its static play/resolve
        // entry points (used by double-click and "Play with…") can reach Services.
        PlaybackCoordinator.init(this);

        // 5. Establish the active Design — this builds and installs the Chrome
        //    (footer, splits, the shared sidebar), so getChrome() is valid afterwards.
        nowPlayingView = new NowPlayingView(viewStack);
        // The now-playing view is owned here (not by a plugin), so register it with
        // the stack ourselves — otherwise navigate("spacify:now-playing") matches
        // nothing and the main view stays blank.
        viewStack.registerView(nowPlayingView);
        setDesign(pickInitialDesign());

        setUndecorated(true);  // remove native title bar + border on all platforms
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setSize(1100, 700);
        setMinimumSize(new Dimension(800, 500));
        setLocationRelativeTo(null);
        // 1px border so the window edge is visible against the desktop
        getRootPane().setBorder(BorderFactory.createLineBorder(new Color(40, 40, 40), 1));

        // ── Theme ─────────────────────────────────────────────────────────────
        getTaste().addChangeListener(config::save);

        Timer themeRebuildTimer = new Timer(250, e -> rebuildTheme());
        themeRebuildTimer.setRepeats(false);
        getTaste().addChangeListener(themeRebuildTimer::restart);

        // Swap the active Service when the selected design style changes.
        getTaste().addChangeListener(() -> {
            
        });

        featureManager.activateFeatures(getViewStack(), getLeftLibraryMenu().getRootNode());


        // Wire every registered media Service for events; the footer/queue follow
        // whichever one PlaybackCoordinator marks active for the current play.
        for (MediaService ms : serviceManager.getServices(MediaService.class)) {
            wireMediaService(ms);
        }

        if (getChrome() != null) applySidebar(userWantsSidebar);
        // Glass-pane resize handler — intercepts edge events, redispatches others
        WindowResizer.install(this);

        // Store pages browse full-width with the side panels collapsed.
        if (getChrome() != null) {
            getChrome().getViewStack().addNavigationListener((uri, b, f) ->
                applyImmersive(uri != null && uri.startsWith("spacify:store:")));
        }
        navigate("spacify:now-playing");
    }

    /**
     * Pick the Design to start with: the persisted/"spot" default if registered,
     * else the first Design any plugin contributed, else null (no Chrome — the
     * window still opens, just without the themed layout).
     */
    private Design pickInitialDesign() {
        Design saved = getTaste().getDesign();      // restored by ConfigManager.load()
        if (saved != null) return saved;
        Design d = designManager.get("wmp1x");      // default look and feel
        if (d == null) d = designManager.get("spot");
        if (d != null) return d;
        var all = designManager.all();
        return all.isEmpty() ? null : all.iterator().next();
    }

    /** Toggle the left leftLibraryMenu; remembers the user's preference. */
    public void toggleSidebar() {
        setSidebarVisible(!userWantsSidebar);
    }

    /** Explicitly show/hide the left leftLibraryMenu; remembers the user's preference. */
    public void setSidebarVisible(boolean visible) {
        userWantsSidebar = visible;
        if (!immersive) applySidebar(visible);
    }

    private void applySidebar(boolean visible) {
        getLeftLibraryMenu().setVisible(visible);
        getLeftSplit().getComponent().setDividerLocation(visible ? 220 : 0);
        getLeftSplit().revalidate();
        getLeftSplit().repaint();
    }

    /** Full-width browsing: collapse both side panels for store views. */
    private void applyImmersive(boolean on) {
        if (on == immersive) return;
        immersive = on;
        if (on) {
            applySidebar(false);
            getMainSplit().getComponent().setDividerLocation(1.0);   // collapse the right Now Playing panel
        } else {
            applySidebar(userWantsSidebar);
            getMainSplit().getComponent().setDividerLocation(880);
        }
        getMainSplit().revalidate();
        getMainSplit().repaint();
    }

    public void navigate(String uri) {
    	getViewStack().navigate(uri);
    }

    private void rebuildTheme() {
        // 1. Reinstall Nimbus to clear its SynthStyleFactory painter cache
        try {
            for (UIManager.LookAndFeelInfo info : UIManager.getInstalledLookAndFeels()) {
                if ("Nimbus".equals(info.getName())) {
                    UIManager.setLookAndFeel(info.getClassName());
                    break;
                }
            }
        } catch (Exception ignored) {}
        // 2. Re-apply our colour overrides to the freshly-installed L&F defaults
        ThemeManager.applyToDefaults();
        // 3. Propagate to all components
        SwingUtilities.updateComponentTreeUI(this);
    }

    /** Connects a MediaService to AppFooter and NowPlayingView. */
    public void wireMediaService(MediaService ms) {
        if (getAppFooter() != null) getAppFooter().setMediaService(ms);
        if (getNowPlayingView() != null) getNowPlayingView().setMediaService(ms);
        // Auto-advance the play queue when a track reaches its natural end.
        ms.addPlaybackListener(new MediaService.PlaybackListener() {
            @Override public void onCompleted() {
                SwingUtilities.invokeLater(() -> PlayQueue.getInstance().next());
            }
        });
    }

}
