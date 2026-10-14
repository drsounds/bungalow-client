package se.spacify.app.wmp.controls;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.FlowLayout;

import java.util.HashMap;
import java.util.Map;

import javax.swing.BorderFactory;
import javax.swing.Icon;
import javax.swing.ImageIcon;
import javax.swing.JMenuItem;
import javax.swing.JPanel;
import javax.swing.JPopupMenu;

import javax.swing.SwingConstants;
import javax.swing.SwingUtilities;
import javax.swing.SwingWorker; 

import se.spacify.controls.GlassPanel;
import se.spacify.controls.GlossyButton;
import se.spacify.controls.Panel;
import se.spacify.controls.TabButton;
import se.spacify.navigation.ViewStack;
import se.spacify.ui.AppHeader;
import se.spacify.ui.MainWindow;
import se.spacify.ui.theme.ThemeManager;
import se.spacify.web.FaviconFetcher;
import se.spacify.web.SiteUri;
import se.spacify.web.StoreCatalog;

public class WMP1XAppHeader extends AppHeader {

	protected TabButton mediaGuideTab;
	protected TabButton nowPlayingTab;
	protected TabButton libraryTab;
	protected GlossyButton storesBtn;
	private final Map<String, Icon> faviconCache = new HashMap<>();
	private Panel navButtons;
	private Panel center;
	/** Build the stores popup from the catalogue, using cached favicons. */
	private JPopupMenu buildStoresMenu() {
		JPopupMenu menu = new JPopupMenu();
		for (StoreCatalog.Store store : StoreCatalog.STORES) {
			JMenuItem item = new JMenuItem(store.name(), faviconCache.get(store.host()));
			item.addActionListener(e -> {
				if (viewStack.getMainWindow() != null)
					viewStack.getMainWindow().navigate(store.uri());
				else
					viewStack.navigate(store.uri());
			});
			menu.add(item);
		}
		return menu;
	}

	/** Fetch each store's favicon off the EDT and cache it for the popup. */
	private void loadFavicons() {
		new SwingWorker<Void, Void>() {
			@Override
			protected Void doInBackground() {
				for (StoreCatalog.Store store : StoreCatalog.STORES) {
					byte[] png = FaviconFetcher.fetch(store.host());
					if (png != null) {
						Icon icon = new ImageIcon(png);
						SwingUtilities.invokeLater(() -> {
							faviconCache.put(store.host(), icon);
							updateStoresButton(viewStack.getCurrentUri());
						});
					}
				}
				return null;
			}
		}.execute();
	}

	/**
	 * While on a store, show its favicon + name in the dropdown; else "Stores ▾".
	 */
	private void updateStoresButton(String uri) {
		if (storesBtn == null)
			return;
		if (uri != null && uri.startsWith(SiteUri.STORE_PREFIX)) {
			String host = SiteUri.host(uri, SiteUri.STORE_PREFIX);
			StoreCatalog.Store store = StoreCatalog.STORES.stream().filter(s -> s.host().equals(host)).findFirst()
					.orElse(null);
			storesBtn.getComponent().setText((store != null ? store.name() : host) + " ▾");
			storesBtn.getComponent().setIcon(host != null ? faviconCache.get(host) : null);
		} else {
			storesBtn.getComponent().setText("Stores ▾");
			storesBtn.getComponent().setIcon(null);
		}
	}

    public WMP1XAppHeader(ViewStack viewStack) {
        super(viewStack);

		// Centre region carries the WMP tab strip along its bottom edge.
		center = new Panel(new BorderLayout());
		center.getComponent().setOpaque(false);

		navButtons = new Panel(new FlowLayout(FlowLayout.LEFT, 4, 0));
		navButtons.getComponent().setOpaque(false);
		// navButtons.add(sidebarToggle);
		navButtons.add(backBtn);
		navButtons.add(forwardBtn);
		navButtons.getComponent().setBorder(BorderFactory.createEmptyBorder(8, 12, 8, 12));

		// WMP-style tab strip, flush with the bottom edge of the nav bar.
		nowPlayingTab = new TabButton("Now Playing");
		nowPlayingTab.getComponent().addActionListener(e -> {
			MainWindow mw = viewStack.getMainWindow();
			if (mw != null) {
				mw.setSidebarVisible(false);
				mw.navigate("spacify:now-playing");
			}
		});
		libraryTab = new TabButton("Library");
		libraryTab.getComponent().addActionListener(e -> {
			MainWindow mw = viewStack.getMainWindow();
			if (mw != null) {
				mw.setSidebarVisible(true);
				mw.navigate("spacify:library");
			}
		});
		mediaGuideTab = new TabButton("Media Guide");
		mediaGuideTab.getComponent().addActionListener(e -> {
			MainWindow mw = viewStack.getMainWindow();
			if (mw != null) {
				mw.setSidebarVisible(true);
				mw.navigate("spacify:store:www.last.fm");
			}
		});
		JPanel tabBar = new JPanel(new FlowLayout(FlowLayout.CENTER, 4, 0));
		tabBar.setOpaque(false);
		tabBar.add(nowPlayingTab.getComponent());
		tabBar.add(libraryTab.getComponent());
		tabBar.add(mediaGuideTab.getComponent());
		center.getComponent().add(tabBar, BorderLayout.SOUTH);
		JPanel right = new JPanel(new FlowLayout(FlowLayout.RIGHT, 5, 5));
		right.setOpaque(false);

		// right.add(searchField);
		GlassPanel storePanel = new GlassPanel();
		storePanel.getComponent().setPreferredSize(new Dimension(300, 46));
		storePanel.getComponent().setLayout(new FlowLayout(FlowLayout.CENTER, 0, 0));
		storePanel.setLeadingDiagonal(true); // sharp left edge, bottom longer than top
		storePanel.setDiagonalInset(65);
		storesBtn = makeNavButton("Stores ▾");
		storesBtn.getComponent().setPreferredSize(new Dimension(160, 32));
		storesBtn.getComponent().setToolTipText("Open a music Service");
		// Transparent & borderless so it floats on the glass field.
		storesBtn.getComponent().setOpaque(false);
		storesBtn.getComponent().setContentAreaFilled(false);
		storesBtn.getComponent().setBorderPainted(false);
		storesBtn.getComponent().setBorder(BorderFactory.createEmptyBorder(4, 8, 4, 8));
		storesBtn.getComponent().setHorizontalAlignment(SwingConstants.LEFT);
		storesBtn.getComponent().addActionListener(e -> buildStoresMenu().show(storesBtn.getComponent(), 0, storesBtn.getComponent().getHeight()));
		storePanel.add(storesBtn);

		add(navButtons, BorderLayout.WEST);
		add(center, BorderLayout.CENTER);
		getComponent().add(right, BorderLayout.EAST);

		right.add(storePanel.getComponent(), BorderLayout.CENTER);

		loadFavicons();

		viewStack.addNavigationListener(this);
		ThemeManager.addChangeListener(this::repaint);
    }
	@Override
	public void onNavigate(String uri, boolean canGoBack, boolean canGoForward) {
        super.onNavigate(uri, canGoBack, canGoForward);
		updateStoresButton(uri);
		nowPlayingTab.getComponent().setSelected(uri != null && uri.startsWith("spacify:now-playing"));
		libraryTab.getComponent().setSelected(uri != null && uri.startsWith("spacify:library"));
	}
}
