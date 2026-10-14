package se.spacify.controls;

import java.awt.Component;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;

import javax.swing.JToolBar;

import se.spacify.ui.theme.ThemeManager;

/**
 * A toolbar container control wrapping a {@link JToolBar}, painted by the active
 * skin via {@link se.spacify.skinning.Skin#paintToolBar}. As a container it offers
 * a small add facade ({@link #add(Control)}, {@link #add(Component)},
 * {@link #addSeparator()}).
 */
public class ToolBar extends Control<JToolBar> {

	protected class Surface extends JToolBar {
		private static final long serialVersionUID = 1L;
		Surface() { super(); }
		@Override
		protected void paintComponent(Graphics g) {
			Graphics2D g2 = (Graphics2D) g.create();
			g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
			getSkin().paintToolBar(ToolBar.this, g2);
			g2.dispose();
		}
	}

	public ToolBar() {
		this.component = new Surface();
		component.setFloatable(false);
		component.setOpaque(true);
		component.setBackground(ThemeManager.getTintColor());
	}
}
