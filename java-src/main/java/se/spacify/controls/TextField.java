package se.spacify.controls;

import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;

import javax.swing.JTextField;

/**
 * A single-line text input control wrapping a {@link JTextField}. Reach the widget
 * through {@link #getComponent()}; the wrapped {@link Surface} paints the skinned
 * field background before the text.
 */
public class TextField extends Control<JTextField> {

	protected class Surface extends JTextField {
		private static final long serialVersionUID = 1L;
		Surface() { super(); }
		Surface(String text) { super(text); }
		@Override
		protected void paintComponent(Graphics g) {
			Graphics2D g2 = (Graphics2D) g.create();
			g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
			getSkin().paintTextField(TextField.this, g2);
			g2.dispose();
			super.paintComponent(g);
		}
	}

	public TextField() {
		this.component = new Surface();
	}

	public TextField(String text) {
		this.component = new Surface(text);
	}
}
