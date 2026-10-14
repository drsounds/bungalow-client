package se.spacify.controls;

import java.awt.Component;

/**
 * A thin {@link Control} over an already-built Swing/AWT {@link Component}. It lets
 * the XUL inflater mount widgets that are not themselves {@code Control}s — e.g. a
 * fully-qualified tag such as {@code <javax.swing.JProgressBar>} resolves to a raw
 * component, which is wrapped here so it can live in the control tree.
 */
public class ComponentControl extends Control<Component> {

	public ComponentControl(Component component) {
		this.component = component;
	}
}
