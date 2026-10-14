package se.spacify.navigation;

import java.awt.BorderLayout;

import se.spacify.controls.Panel;

/**
 * A navigable screen. A View is a {@link Panel} control, so the {@link ViewStack}
 * mounts the view's {@link #getComponent() component} as a card and shows/hides it
 * automatically — no manual component swapping. Subclasses render into the panel
 * via the {@link Panel} container facade ({@link #add}, {@link #setLayout}, …).
 */
public abstract class View extends Panel {

	private final ViewStack viewStack;

	public View(ViewStack viewStack) {
		this.viewStack = viewStack;
		getComponent().setLayout(new BorderLayout());
	}

	public ViewStack getViewStack() {
		return viewStack;
	}

	public abstract boolean acceptsUri(String uri);

	public abstract void navigate(String uri);

	public String getName() {
		return "";
	}

	public void onShow() {}

	public void onHide() {}

	/**
	 * Whether this view manages its own internal navigation history (e.g. an
	 * embedded browser). When true, {@link ViewStack}'s back/forward delegate
	 * to {@link #goBack()}/{@link #goForward()} before touching the view stack.
	 */
	public boolean handlesHistory() { return false; }

	public boolean canGoBack()    { return false; }
	public boolean canGoForward() { return false; }
	public void    goBack()       {}
	public void    goForward()    {}
}
