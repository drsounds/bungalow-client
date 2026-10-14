package se.spacify.app.wmp.skin;

import java.awt.Color;
import java.awt.GradientPaint;
import java.awt.Graphics2D;

import java.awt.RadialGradientPaint;
import java.awt.Shape;
import java.awt.geom.Ellipse2D;
import java.awt.geom.Path2D;
import java.awt.geom.Point2D;

import javax.swing.ButtonModel;
import javax.swing.JComponent;
import javax.swing.JPanel;

import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;
import se.spacify.controls.Control;
import se.spacify.controls.GlassPanel;
import se.spacify.controls.GlossyButton;
import se.spacify.controls.Panel;
import se.spacify.controls.SplitPane.SplitPaneDivider;
import se.spacify.controls.TabButton;
import se.spacify.controls.Table;
import se.spacify.controls.ToolBar;
import se.spacify.controls.ToolButton;
import se.spacify.controls.Tree;
import se.spacify.controls.VerticalPanel;
import se.spacify.graphics.StretchableRadialGradient;

import se.spacify.ui.theme.ColorUtils;
import se.spacify.ui.theme.ThemeManager;

public class WMP11Skin extends WMPSkin {

	private static final int ARC = 12;
	@Override
	public void paintTopBar(Panel control, Graphics2D g2) {
        int w = control.getWidth(), h = control.getHeight();
		g2.setPaint(new GradientPaint(0, 0, Color.BLACK, 0, h, ColorUtils.saturate(ThemeManager.getTintColor(), 0.5f)));
		g2.fillRect(0, 0, w, h);
	}
	@Override
	public void paintHeader(Panel control, Graphics2D g2) {
        int w = control.getWidth(), h = control.getHeight();
        g2.setPaint(Color.BLACK);
		g2.fillRect(0, 0, w, h);
		
		g2.setPaint(new GradientPaint(0, 0, ColorUtils.saturate(ThemeManager.getTintColor(), 0.5f), 0, h, new Color(0, 0, 0 ,0)));
		g2.fillRect(0, 0, w, h / 2);
	
		g2.setPaint(new GradientPaint(0, h / 2, new Color(0, 0, 0, 0), 0, h,  ThemeManager.getTintColor()));
		g2.fillRect(0, h / 2, w, (h / 2));
	}
	@Override
	public void paintFooter(Panel control, Graphics2D g2) {
        int w = control.getWidth(), h = control.getHeight();
       
        g2.setPaint(Color.BLACK);
		g2.fillRect(0, 0, w, h);
		
		g2.setPaint(new GradientPaint(0, 0, ColorUtils.saturate(ThemeManager.getTintColor(), 0.5f), 0, h, new Color(0, 0, 0, 0)));
		g2.fillRect(0, 0, w, h / 2);
	
		g2.setPaint(new GradientPaint(0, h / 2, new Color(0, 0, 0, 0), 0, h,  ThemeManager.getTintColor()));
		g2.fillRect(0, h / 2, w, (h / 2));
	}
	@Override
	public void paintTabButton(TabButton control, Graphics2D g2) {
		int w = control.getComponent().getWidth(), h = control.getComponent().getHeight();

		// Top-rounded, bottom-sharp shape flush with the panel's bottom edge.
		Path2D tab = new Path2D.Float();
		tab.moveTo(0, h);
		tab.lineTo(0, ARC);
		tab.quadTo(0, 0, ARC, 0);
		tab.lineTo(w - ARC, 0);
		tab.quadTo(w, 0, w, ARC);
		tab.lineTo(w, h);
		tab.closePath();


	 	if (control.getComponent().isSelected() || control.isPressedState()) {
			paintGlossy(g2, false, 0, 0, control.getComponent().getWidth(), control.getComponent().getHeight(), false, true, false);
		} else if (control.isHovered()) {
			g2.setColor(new Color(255, 255, 255, 45));
			if (control.getOrientation() == TabButton.ORIENTATION_HORIZONTAL) {
				g2.fill(tab);
			} else {
				g2.fillRect(0, 0, w, h);
			}
		} else {
		}
	 	
	}
	
	@Override
	public void paintGlossyButton(GlossyButton control, Graphics2D g2, int x, int y, int d) {
		// TODO Auto-generated method stub
		paintGlossy(g2, true, x, y, control.getComponent().getWidth(), control.getComponent().getHeight(), control.getHovered(), control.getPrimary(), control.getPressed());
	}
	
	private void paintGlossy(Graphics2D g2, boolean round, int x, int y, int width, int height, boolean hovered, boolean primary, boolean pressed) {
		Color accent = ThemeManager.getAccentForegroundColor();
		Color tint = ThemeManager.getTintColor();

		Color glowColor = new Color(255, 255, 255, 0);
		if (primary) {
			glowColor = ColorUtils.saturate(tint, 125.2f);
		} 
		Color[] lights = new Color[] { ColorUtils.alpha(ColorUtils.lighten(glowColor, 25f), 255f), ColorUtils.alpha(ColorUtils.lighten(glowColor, 25f), 0f) };
		
		// Darker outer rim, slightly inset face sits on top of it.
		//Ellipse2D outer = new Ellipse2D.Float(x, y, d, d);
		
		g2.setPaint(new GradientPaint(x, y, ThemeManager.tintDark(0.9f), x, y + height, ThemeManager.tintDark(0.5f)));
		//g2.fill(outer);

		Ellipse2D face = new Ellipse2D.Float(x - 4, y - 4, width + 8, height + 8);
		if (round) {
			g2.setClip(face);
		}
		g2.setPaint(new GradientPaint(0, 0, new Color(0, 0, 0, 0), width,
				y + height / 2, new Color(255, 255, 255, 127)));
		g2.fillRect(x, y, width + 4, height + 4);

		Ellipse2D face2 = new Ellipse2D.Float(x, y, width, height);
		// Base vertical gradient — brighter when hovered, dimmer/inverted when pressed.
		Color top, bottom;
		if (pressed) {
			top = ThemeManager.accentDark(0.2f);
			bottom = ThemeManager.scaleRgb(accent, 0.9f);
		} else if (hovered) {
			top = ThemeManager.accentLight(1.7f);
			bottom = tint;
		} else {
			top = ThemeManager.accentLight(1.4f);
			bottom = tint;
		}
		if (primary) {
			top = ColorUtils.saturate(top, 255);
			bottom = ColorUtils.saturate(bottom, 255);
		}
		if (primary) {

			g2.setPaint(tint);
		} else {
			g2.setPaint(new GradientPaint(x + 2, y + 2, top, x, y + height, bottom));
		}
		Shape oldClip = g2.getClip();
		if (round) {
			g2.fill(face2);
	
			g2.setClip(face2);
			
		} else {
			g2.fillRect(x, y, width, height);
		}
		g2.setPaint(
			new RadialGradientPaint(
				width / 2f,
				height * 1f,
				25,
				new float[] { 0.0f, 1 },
				lights
			)
		);
		g2.fillRect(x, y + height -20, width, 20);
		
		g2.setPaint(
			new StretchableRadialGradient(
				new Point2D.Float(width / 2f,
				height / 2f),
				15,
				new float[] { 0.0f, 1 },
				new Color[] { 
					new Color(255, 255, 255, 0),
					new Color(255, 255, 255, 127)
				},
				width,
				height,
				0
			)

		);
		g2.fillRect(0, 0, width, height / 2);
		if (round) {
			g2.setClip(oldClip);
		}
		// Thin inner ring to crisp up the rim/face boundary.
		if (round) {
			g2.setStroke(new java.awt.BasicStroke(1f));
			g2.setColor(new Color(255, 255, 255, 60));
			g2.draw(new Ellipse2D.Float(x + 0.5f, y + 0.5f, width - 1, height - 1));
		}
	}
	

	@Override
	public void paintGlassPanel(GlassPanel control, Graphics2D g2) {
		int w = control.getComponent().getWidth(), h = control.getComponent().getHeight();
		Path2D shape = control.shape(w, h);

		// Glossy white overlays, confined to the shape.
		g2.setClip(shape);
        g2.setPaint(Color.BLACK);
		g2.fillRect(0, 0, w, h);
		// Base accent gradient, antialiased to the rounded/diagonal outline.
		g2.setPaint(new GradientPaint(0, 0, ThemeManager.tintLight(2f), 0, h, ThemeManager.getTintColor()));
		g2.fill(shape);

		g2.setPaint(new GradientPaint(0, 0, ThemeManager.getTintColor(), 0, h, new Color(0, 0, 0, 0)));
		g2.fillRect(0, 0, w, h / 2);
	
		g2.setPaint(new GradientPaint(0, h / 2, new Color(0, 0, 0, 0), 0, h,  ThemeManager.getTintColor()));
		g2.fillRect(0, h / 2, w, (h / 2));
	}

	@Override
	public void paintSplitPaneDivider(SplitPaneDivider control, Graphics2D g2) {
		int w = control.getWidth(), h = control.getHeight();
		if (ThemeManager.isDarkMode()) {
			g2.setPaint(new Color(235, 234, 219));
			g2.fillRect(0, 0, w, h);
		}
	}

	@Override
	public void paintPlaylist(Panel control, Graphics2D g2) {
		// TODO Auto-generated method stub
		int w = control.getWidth(), h = control.getHeight();
        g2.setPaint(ThemeManager.getBackground());
        g2.fillRect(0, 0, w, h);
	}
	@Override
	public void paintToolBar(ToolBar control, Graphics2D g2) {
		int w = control.getComponent().getWidth(), h = control.getComponent().getHeight();
		g2.setPaint(ThemeManager.getBackground());
		g2.fillRect(0, 0, w, h);
	}

	@Override
	public void paintTree(Tree tree, Graphics2D g2) {

	}

	@Override
	public void paintControl(Control<? extends JComponent> control, Graphics2D g2) {

	}

	@Override
	public void paintToolButton(ToolButton control, Graphics2D g2) {
		int w = control.getComponent().getWidth(), h = control.getComponent().getHeight();
		Color background = ThemeManager.getBackground();
        ButtonModel model = control.getComponent().getModel();
		g2.setPaint(background);
		g2.fillRect(0, 0, w, h);
		if (model.isPressed()) {
			background = ColorUtils.darken(background, 0.9f);
			g2.setPaint(background);
			g2.fillRect(0, 0, w, h);
			background = ColorUtils.darken(background, 0.5f);
			g2.setPaint(background);
			g2.fillRect(0, 0, w, 2);
			g2.fillRect(0, 0, 2, h);
			g2.setPaint(ColorUtils.lighten(background, 5));
			g2.fillRect(w - 2, 0, 2, h);
			g2.fillRect(0, h - 2, 2, h);
		} else if (model.isRollover()) {
			background = ColorUtils.darken(background, 2f);
			g2.setPaint(background);
			g2.fillRect(0, 0, w, h);
			background = ColorUtils.lighten(background, 2);
			g2.setPaint(background);
			g2.fillRect(0, 0, w, h);
			g2.setPaint(ColorUtils.darken(background, 3));
			g2.fillRect(0, 0, w, 2);
			g2.fillRect(0, 0, 2, h);
			g2.setPaint(ColorUtils.lighten(background, 3));
			g2.fillRect(w, 0, 2, h);
			g2.fillRect(0, h, 2, h);
			
		}
	}

	@Override
	public void paintTableHeader(Table table, int width, int height, Graphics2D g2) {
		// TODO Auto-generated method stub
		g2.setPaint(new Color(235, 234, 219));
		g2.fillRect(0, 0, width, height);
		
	}
	@Override
	public void paintVerticalPanel(VerticalPanel control, Graphics2D g2) {
		// TODO Auto-generated method stub
		int w = control.getComponent().getWidth(), h = control.getComponent().getHeight();

		g2.setPaint(Color.BLACK);
		g2.fillRect(0, 0, w, h);
		
		g2.setPaint(new GradientPaint(0, 0, ColorUtils.saturate(ThemeManager.getTintColor(), 0.5f), w, 0, new Color(0, 0, 0 ,0)));
		g2.fillRect(0, 0, w, h / 2);
	
		g2.setPaint(new GradientPaint(0, h / 2, new Color(0, 0, 0, 0), w, 0,  ThemeManager.getTintColor()));
		g2.fillRect(0, h / 2, w, (h / 2));
	}
	@Override
	public String getId() {
		// TODO Auto-generated method stub
		return "wmp11";
	}
	@Override
	public String getName() {
		// TODO Auto-generated method stub
		return "WMP 11";
	}
	@Override
	public void onRegister(AspectManager<? extends Aspect> aspectManager) {
		// TODO Auto-generated method stub
	}
}
