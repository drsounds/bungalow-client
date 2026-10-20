package se.bungalow.controls;

import javax.swing.JScrollPane;

public class ScrollPane extends Control<JScrollPane> {
	public ScrollPane() {
		this.component = new JScrollPane();
	}

	/** Wraps {@code content}'s component as the scroll pane's viewport view. */
	public ScrollPane(Control<?> content) {
		this.component = new JScrollPane(content.getComponent());
		children.add(content);
		content.setParent(this);
	}
}
