package se.spacify.controls;

import java.awt.Graphics;
import java.awt.Graphics2D;

import javax.swing.BorderFactory;

import se.spacify.ui.MainWindow;

/**
 * The WMP-style vertical navigation strip (Now Playing / Library / Media Guide),
 * painted by the active skin ({@link se.spacify.skinning.Skin#paintVerticalPanel}).
 */
public class VerticalPanel extends Panel {

	private final TabButton nowPlayingTab;
	private final TabButton libraryTab;
	private final TabButton mediaGuideTab;

	public VerticalPanel() {
		getComponent().setOpaque(false);
		getComponent().setBorder(BorderFactory.createEmptyBorder(0, 80, 0, 0));
		// WMP-style tab strip, flush with the bottom edge of the nav bar.
		nowPlayingTab = new TabButton("Now Playing");
		nowPlayingTab.getComponent().addActionListener(e -> {
			MainWindow mw = getMainWindow().getViewStack().getMainWindow();
			if (mw != null) {
				mw.setSidebarVisible(false);
				mw.navigate("spacify:now-playing");
			}
		});
		libraryTab = new TabButton("Library");
		libraryTab.getComponent().addActionListener(e -> {
			MainWindow mw = getMainWindow().getViewStack().getMainWindow();
			if (mw != null) {
				mw.setSidebarVisible(true);
				mw.navigate("spacify:library");
			}
		});
		mediaGuideTab = new TabButton("Media Guide");
		mediaGuideTab.getComponent().addActionListener(e -> {
			MainWindow mw = getMainWindow().getViewStack().getMainWindow();
			if (mw != null) {
				mw.setSidebarVisible(true);
				mw.navigate("spacify:store:www.last.fm");
			}
		});

		add(nowPlayingTab);
		add(libraryTab);
		add(mediaGuideTab);
	}

	@Override
	protected void paintSurface(Graphics g) {
		Graphics2D g2 = (Graphics2D) g.create();
		getSkin().paintVerticalPanel(this, g2);
		g2.dispose();
		super.paintSurface(g);
	}
}
