package se.spacify.controls;

import se.spacify.ui.theme.ThemeManager;

import javax.swing.BorderFactory;
import javax.swing.SwingConstants;
import javax.swing.SwingUtilities;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;

/**
 * A circular, glossy push button in the Frutiger Aero / Aqua idiom of Windows
 * Media Player 10. A {@link Button} whose face is painted by the active skin
 * ({@link se.spacify.skinning.Skin#paintGlossyButton}); text and icon are drawn
 * over it. Hover/press feedback is tracked explicitly (as on {@link TabButton}).
 */
public class GlossyButton extends Button {

	private static final int DEFAULT_DIAMETER = 64;

	private int diameter = DEFAULT_DIAMETER;
	private boolean hovered;
	private boolean pressed;

	public boolean getHovered() { return hovered; }
	public boolean getPressed() { return pressed; }

	public GlossyButton() {
		this(null);
	}

	public GlossyButton(String text) {
		super(text);
		component.setContentAreaFilled(false);
		component.setBorderPainted(false);
		component.setFocusPainted(false);
		component.setOpaque(false);
		component.setRolloverEnabled(true);
		component.setHorizontalAlignment(SwingConstants.CENTER);
		component.setForeground(Color.WHITE);
		component.setBorder(BorderFactory.createEmptyBorder());

		java.awt.event.MouseAdapter mouse = new java.awt.event.MouseAdapter() {
			@Override public void mouseEntered(java.awt.event.MouseEvent e) { hovered = true; repaint(); }
			@Override public void mouseExited(java.awt.event.MouseEvent e)  { hovered = false; pressed = false; repaint(); }
			@Override public void mousePressed(java.awt.event.MouseEvent e) {
				if (SwingUtilities.isLeftMouseButton(e)) { pressed = true; repaint(); }
			}
			@Override public void mouseReleased(java.awt.event.MouseEvent e) { pressed = false; repaint(); }
		};
		component.addMouseListener(mouse);
		ThemeManager.addChangeListener(this::repaint);
	}

	/** Diameter of the circular face in pixels; also drives the preferred size. */
	public void setDiameter(int d) {
		this.diameter = Math.max(16, d);
		revalidate();
		repaint();
	}

	public int getDiameter() {
		return diameter;
	}

	@Override
	protected Dimension preferredSize(Dimension dflt) {
		return new Dimension(diameter, diameter);
	}

	@Override
	protected void paintSurface(Graphics g) {
		Graphics2D g2 = (Graphics2D) g.create();
		g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
		// Largest centred circle that fits, leaving 1px so the antialiased edge
		// isn't clipped.
		int d = Math.min(getComponent().getWidth(), getComponent().getHeight()) - 2;
		if (d > 0) {
			int x = (getComponent().getWidth() - d) / 2;
			int y = (getComponent().getHeight() - d) / 2;
			getSkin().paintGlossyButton(this, g2, x, y, d);
		}
		g2.dispose();
		super.paintSurface(g);   // text / icon, drawn over the glossy face
	}
}
