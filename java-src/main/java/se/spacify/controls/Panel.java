package se.spacify.controls;

import java.awt.Graphics;
import java.awt.LayoutManager;

import javax.swing.JPanel;

/**
 * The base container control: a clean {@link Control} over a {@link JPanel}.
 * Children are added with {@link #add(Control)}; all other configuration (layout,
 * borders, opacity, geometry, …) is done through {@link #getComponent()}.
 *
 * <p>Custom painting is done by overriding {@link #paintSurface(Graphics)} rather
 * than a Swing {@code paintComponent}; the wrapped {@link Surface} routes painting
 * back here so subclasses never touch Swing directly.
 */
public class Panel extends Control<JPanel> {

	/** The wrapped JPanel; routes painting back to {@link Panel#paintSurface}. */
	protected class Surface extends JPanel {
		private static final long serialVersionUID = 1L;
		Surface() { super(); }
		Surface(LayoutManager layout) { super(layout); }
		@Override
		protected void paintComponent(Graphics g) {
			Panel.this.paintSurface(g);
		}
		/** The standard JPanel painting, for {@link Panel#paintSurface} to call. */
		void superPaint(Graphics g) {
			super.paintComponent(g);
		}
	}

	public Panel() {
		this.component = new Surface();
	}

	public Panel(LayoutManager layout) {
		this.component = new Surface(layout);
	}

	/**
	 * Paint this panel's surface. Default does the standard panel painting; override
	 * to draw a skinned background (call {@code super.paintSurface(g)} to keep it).
	 */
	protected void paintSurface(Graphics g) {
		((Surface) component).superPaint(g);
	}
}
