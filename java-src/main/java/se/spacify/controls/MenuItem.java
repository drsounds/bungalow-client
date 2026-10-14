package se.spacify.controls;

import javax.swing.JMenuItem;

/**
 * A menu item control wrapping a {@link JMenuItem}. Reach the widget through
 * {@link #getComponent()}.
 */
public class MenuItem extends Control<JMenuItem> {

	public MenuItem() {
		this.component = new JMenuItem();
	}

	public MenuItem(String text) {
		this.component = new JMenuItem(text);
	}
}
