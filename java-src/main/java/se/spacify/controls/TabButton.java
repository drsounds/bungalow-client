package se.spacify.controls;

import se.spacify.ui.theme.ThemeManager;

import javax.swing.SwingUtilities;
import javax.swing.BorderFactory;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;

/**
 * A WMP-style tab: a {@link ToggleButton} whose rounded-top shape is painted by
 * the active skin ({@link se.spacify.skinning.Skin#paintTabButton}). Hover/press
 * are tracked explicitly because the Synth-based Nimbus L&amp;F doesn't reliably
 * repaint a custom-painted, non-opaque button on those transitions.
 */
public class TabButton extends ToggleButton {

	public static final String ORIENTATION_HORIZONTAL = "horizontal";
	public static final String ORIENTATION_VERTICAL = "vertical";

	private boolean hovered;
	private boolean pressed;
	private String orientation = ORIENTATION_HORIZONTAL;

	public TabButton(String text) {
		super(text);
		component.setContentAreaFilled(false);
		component.setBorderPainted(false);
		component.setFocusPainted(false);
		component.setOpaque(false);
		component.setRolloverEnabled(true);
		component.setBorder(BorderFactory.createEmptyBorder(10, 32, 8, 32));
		component.setFont(component.getFont().deriveFont(14f));
		component.setForeground(ThemeManager.getForeground());

		MouseAdapter mouse = new MouseAdapter() {
			@Override public void mouseEntered(MouseEvent e) { hovered = true; repaint(); }
			@Override public void mouseExited(MouseEvent e)  { hovered = false; pressed = false; repaint(); }
			@Override public void mousePressed(MouseEvent e) {
				// Show the active (pressed) state immediately; navigation is deferred
				// to release by the action listener.
				if (SwingUtilities.isLeftMouseButton(e)) { pressed = true; repaint(); }
			}
			@Override public void mouseReleased(MouseEvent e) { pressed = false; repaint(); }
		};
		component.addMouseListener(mouse);

		ThemeManager.addChangeListener(() -> {
			component.setForeground(component.isSelected() ? Color.WHITE : ThemeManager.accentDark(0.2f));
			repaint();
		});
	}

	public String getOrientation() { return orientation; }
	public void setOrientation(String value) { orientation = value; }

	/** True while the pointer is over the tab. */
	public boolean isHovered() { return hovered; }

	/** True while the tab is being held down (before release). */
	public boolean isPressedState() { return pressed; }

	@Override
	protected Color foreground(Color dflt) {
		// The skin fills the tab with the active background while selected or
		// pressed, so keep the label white in both cases for legibility.
		if (pressed && !component.isSelected()) return Color.WHITE;
		return dflt;
	}

	@Override
	protected Dimension preferredSize(Dimension d) {
		return new Dimension(d.width, 50);
	}

	@Override
	protected void paintSurface(Graphics g) {
		Graphics2D g2 = (Graphics2D) g.create();
		g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
		getSkin().paintTabButton(this, g2);
		g2.dispose();
		super.paintSurface(g);   // draws text/icon (content area is disabled)
	}
}
