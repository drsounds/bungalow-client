package se.bungalow.ui.chrome;
import java.awt.Component;

import javax.swing.BoxLayout;

import se.bungalow.aspect.Aspect;
import se.bungalow.controls.Panel;
import se.bungalow.controls.SplitPane;
import se.bungalow.navigation.ViewStack;

import se.bungalow.ui.AppFooter;
import se.bungalow.ui.AppHeader;
import se.bungalow.ui.LeftLibraryMenu;
import se.bungalow.ui.LeftMenuPanel;

import se.bungalow.ui.TopBar;
import se.bungalow.app.media.views.NowPlayingView;
	
public abstract class Chrome extends Panel implements Aspect {
	public void onDestroy() {
		
	}
	protected ViewStack viewStack; 
	public void setViewStack(ViewStack value) {
		viewStack = value;
	} 
    protected AppFooter appFooter;
    protected NowPlayingView nowPlayingView;
    protected LeftLibraryMenu leftLibraryMenu;
	public void setLeftLibraryMenu(LeftLibraryMenu value) {
		leftLibraryMenu = value;
	}

    public ViewStack getViewStack() { return viewStack; }
    public AppFooter getAppFooter() { return appFooter; }
    public NowPlayingView getNowPlayingView() { return nowPlayingView; }
    public LeftLibraryMenu getLeftLibraryMenu() { return leftLibraryMenu; }
  
	protected SplitPane leftSplit;
	public SplitPane getLeftSplit() {
		return leftSplit;
	}
	protected SplitPane mainSplit;
	public SplitPane getMainSplit() {
		return mainSplit;
	}
    protected boolean userWantsSidebar = true;  // user's manual show/hide preference
    protected boolean immersive = false;        // full-width store browsing

	protected Panel appPanel;
	public Panel getAppPanel() {
		return appPanel;
	}
	public void setAppPanel(Panel value) {
		appPanel = value;
	}

	protected LeftMenuPanel leftMenuPanel;
	public LeftMenuPanel getLeftMenuPanel() {
		return leftMenuPanel;
	}
	public void setLeftMenuPanel(LeftMenuPanel value) {
		leftMenuPanel = value;
	}

	protected Panel centerPanel; 
	public Panel getCenterPanel() {
		return  centerPanel;
	}
	public void setCenterPanel(Panel value) {
		centerPanel = value;
	}
	protected TopBar topBar;
	public TopBar getTopBar() {
		return topBar;
	}
	public void setTopBar(TopBar value) {
		topBar = value;
	}

	protected AppHeader appHeader;

	public AppHeader getAppHeader() {
		return appHeader;
	}

	protected Panel topSpacing;
	public Panel getTopSpacing() {
		return topSpacing;
	}

    public Chrome() {
    }

	public void build() {
		for (Component comp : getComponent().getComponents()) {
			getComponent().remove(comp);
		}
		getComponent().setLayout(new BoxLayout(getComponent(), BoxLayout.LINE_AXIS));
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

    public void applySidebar(boolean visible) {
        leftLibraryMenu.setVisible(visible);
        leftSplit.getComponent().setDividerLocation(visible ? 220 : 0);
        leftSplit.revalidate();
		leftSplit.getComponent().setDividerSize(1);
        leftSplit.repaint();
    }

    /** Full-width browsing: collapse both side panels for store views. */
    public void applyImmersive(boolean on) {
        if (on == immersive) return;
        immersive = on;
        if (on) {
            applySidebar(false);
            mainSplit.getComponent().setDividerLocation(1.0);   // collapse the right Now Playing panel
        } else {
            applySidebar(userWantsSidebar);
            mainSplit.getComponent().setDividerLocation(880);
        }
        mainSplit.revalidate();
        mainSplit.repaint();
    }

    public void navigate(String uri) {
    	viewStack.navigate(uri);
    } 

}
