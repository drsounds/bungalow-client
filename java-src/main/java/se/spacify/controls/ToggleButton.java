package se.spacify.controls;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;

import javax.swing.JToggleButton;
import javax.swing.SwingUtilities;

import se.spacify.ui.theme.ThemeManager;

/**
 * A two-state toggle control wrapping a {@link JToggleButton}. Subclasses
 * (e.g. {@link TabButton}) customise painting and metrics through the
 * {@link #paintSurface}, {@link #foreground} and {@link #preferredSize} hooks
 * rather than touching Swing directly.
 */
public class ToggleButton extends Control<JToggleButton> {

	protected class Surface extends JToggleButton {
		private static final long serialVersionUID = 1L;
		Surface() { super(); }
		Surface(String text) { super(text); }
		@Override
		protected void paintComponent(Graphics g) {
			ToggleButton.this.paintSurface(g);
		}
		@Override
		public Color getForeground() {
			return ToggleButton.this.foreground(super.getForeground());
		}
		@Override
		public Dimension getPreferredSize() {
			return ToggleButton.this.preferredSize(super.getPreferredSize());
		}
		void superPaint(Graphics g) { super.paintComponent(g); }
	}

	public ToggleButton() {
		this.component = new Surface();
		init();
	}

	public ToggleButton(String text) {
		this.component = new Surface(text);
		init();
	}

	private void init() {
		component.getModel().addChangeListener(e -> repaint());
		java.awt.event.MouseAdapter mouse = new java.awt.event.MouseAdapter() {
			@Override public void mouseEntered(java.awt.event.MouseEvent e) { repaint(); }
			@Override public void mouseExited(java.awt.event.MouseEvent e)  { repaint(); }
			@Override public void mousePressed(java.awt.event.MouseEvent e) {
				if (SwingUtilities.isLeftMouseButton(e)) { repaint(); }
			}
			@Override public void mouseReleased(java.awt.event.MouseEvent e) { repaint(); }
		};
		component.addMouseListener(mouse);
		ThemeManager.addChangeListener(this::repaint);
	}

	/** Override to custom-paint; default does the standard toggle painting. */
	protected void paintSurface(Graphics g) {
		((Surface) component).superPaint(g);
	}

	/** Override to adjust the foreground used while painting. */
	protected Color foreground(Color dflt) {
		return dflt;
	}

	/** Override to adjust the preferred size. */
	protected Dimension preferredSize(Dimension dflt) {
		return dflt;
	}

}
