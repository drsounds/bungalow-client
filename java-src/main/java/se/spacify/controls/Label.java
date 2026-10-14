package se.spacify.controls;

import java.awt.Graphics;

import javax.swing.JLabel;

/**
 * A text/icon label control wrapping a {@link JLabel}. Reach the widget through
 * {@link #getComponent()}; custom painting is done by overriding
 * {@link #paintSurface(Graphics)} (the wrapped {@link Surface} routes here).
 */
public class Label extends Control<JLabel> {

	/** The wrapped JLabel; routes painting back to {@link Label#paintSurface}. */
	protected class Surface extends JLabel {
		private static final long serialVersionUID = 1L;
		Surface() { super(); }
		Surface(String text) { super(text); }
		@Override
		protected void paintComponent(Graphics g) {
			Label.this.paintSurface(g);
		}
		void superPaint(Graphics g) {
			super.paintComponent(g);
		}
	}

	public Label() {
		this.component = new Surface();
	}

	public Label(String text) {
		this.component = new Surface(text);
	}

	/** Override to custom-paint; default does the standard label painting. */
	protected void paintSurface(Graphics g) {
		((Surface) component).superPaint(g);
	}
}
