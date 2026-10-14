package se.spacify.controls;

import javax.swing.BoxLayout;

/**
 * A vertical box container: a {@link Panel} that stacks its children top-to-bottom.
 */
public class VBox extends Panel {

	public VBox() {
		super();
		getComponent().setLayout(new BoxLayout(getComponent(), BoxLayout.PAGE_AXIS));
	}
}
