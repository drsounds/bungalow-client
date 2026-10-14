package se.spacify.controls;

import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.geom.Path2D;

/**
 * Windows-Media-Player-style glossy panel with rounded corners, painted by the
 * active skin ({@link se.spacify.skinning.Skin#paintGlassPanel}). Either vertical
 * edge can optionally be rendered as a sharp diagonal instead of a rounded corner
 * — see {@link #setLeadingDiagonal(boolean)} / {@link #setTrailingDiagonal(boolean)}.
 * A diagonal edge slants so its bottom is longer than its top by
 * {@link #setDiagonalInset(int)} pixels.
 */
public class GlassPanel extends Panel {

	private int arc = 8;
	private int diagonalInset = 18;
	private boolean leadingDiagonal = false;   // left edge
	private boolean trailingDiagonal = false;  // right edge

	public GlassPanel() {
		super();
		getComponent().setOpaque(false);
	}

	@Override
	protected void paintSurface(Graphics g) {
		Graphics2D g2 = (Graphics2D) g.create();
		g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
		getSkin().paintGlassPanel(this, g2);
		g2.dispose();
		super.paintSurface(g);
	}

	public void setArc(int arc)                  { this.arc = arc; repaint(); }
	public void setDiagonalInset(int inset)      { this.diagonalInset = inset; repaint(); }
	public void setLeadingDiagonal(boolean on)   { this.leadingDiagonal = on; repaint(); }
	public void setTrailingDiagonal(boolean on)  { this.trailingDiagonal = on; repaint(); }

	/** The panel outline: rounded corners, with optional diagonal side edges. */
	public Path2D shape(int w, int h) {
		int a = Math.min(arc, Math.min(w, h) / 2);
		int s = Math.min(diagonalInset, w / 2);
		Path2D p = new Path2D.Float();

		// top-left start
		p.moveTo(leadingDiagonal ? s : a, 0);

		// top edge → top-right
		if (trailingDiagonal) {
			p.lineTo(w - s, 0);
		} else {
			p.lineTo(w - a, 0);
			p.quadTo(w, 0, w, a);
		}

		// right edge → bottom-right
		if (trailingDiagonal) {
			p.lineTo(w, h);                 // diagonal: bottom longer than top
		} else {
			p.lineTo(w, h - a);
			p.quadTo(w, h, w - a, h);
		}

		// bottom edge → bottom-left
		if (leadingDiagonal) {
			p.lineTo(0, h);                 // diagonal: bottom longer than top
		} else {
			p.lineTo(a, h);
			p.quadTo(0, h, 0, h - a);
		}

		// left edge → back to start
		if (leadingDiagonal) {
			p.lineTo(s, 0);
		} else {
			p.lineTo(0, a);
			p.quadTo(0, 0, a, 0);
		}

		p.closePath();
		return p;
	}
}
