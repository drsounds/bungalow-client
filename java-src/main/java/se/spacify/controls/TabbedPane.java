package se.spacify.controls;

import javax.swing.JTabbedPane;

/**
 * A tabbed container control wrapping a {@link JTabbedPane}. Reach the widget
 * through {@link #getComponent()}; child controls are added (with a tab title) via
 * {@link #addTab(String, Control)}.
 */
public class TabbedPane extends Control<JTabbedPane> {

	public TabbedPane() {
		this.component = new JTabbedPane();
	}

	/** Add a child control as a titled tab (mounts {@code child.getComponent()}). */
	public Control<JTabbedPane> addTab(String title, Control<?> child) {
		children.add(child);
		child.setParent(this);
		if (child.getComponent() != null) component.addTab(title, child.getComponent());
		return this;
	}
}
