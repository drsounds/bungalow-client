package se.spacify.controls;

import java.awt.Graphics;
import java.awt.Graphics2D;

import javax.swing.Icon;

/**
 * A toolbar button: a {@link Button} that also paints the skinned tool-button
 * background ({@link se.spacify.skinning.Skin#paintToolButton}).
 */
public class ToolButton extends Button {

	public ToolButton() { super(); }
	public ToolButton(String text) { super(text); }
	public ToolButton(Icon icon, String text) { super(icon, text); }
	public ToolButton(Icon icon) { super(icon); }

	@Override
	protected void paintSurface(Graphics g) {
		Graphics2D g2 = (Graphics2D) g.create();
		getSkin().paintToolButton(this, g2);
		g2.dispose();
		super.paintSurface(g);
	}
}
