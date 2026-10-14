package se.spacify.app.spot.skin;

import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.LinearGradientPaint;
import java.awt.Point;

import javax.swing.JComponent;
import javax.swing.JPanel;

import se.spacify.controls.Button;
import se.spacify.controls.Panel;
import se.spacify.controls.SplitPane.SplitPaneDivider;
import se.spacify.controls.Table;
import se.spacify.controls.TextField;
import se.spacify.controls.ToolBar;
import se.spacify.controls.Tree;

import se.spacify.navigation.ViewStack;
import se.spacify.ui.LeftLibraryMenu;
import se.spacify.ui.LeftMenuPanel;
import se.spacify.ui.theme.ColorUtils;
import se.spacify.ui.theme.Taste;
import se.spacify.ui.theme.ThemeManager;

public class Spot09Skin extends SpotSkin {

    @Override
    public String getId() {
        // TODO Auto-generated method stub
        return "spot09";
    }

    @Override
    public void paintHeader(Panel header, Graphics2D g2) {
        // TODO Auto-generated method stub
        Taste taste = se.spacify.ui.MainWindow.getInstance().getTaste();
        Color tintColor = taste.getTintColor();
        int w = header.getWidth(), h = header.getHeight();
        g2.setPaint(new LinearGradientPaint(new Point(0, 0), new Point(0, h), new float[] {0, 1}, new Color[] { ColorUtils.darken(tintColor, 0.8f), ColorUtils.darken(tintColor, 0.5f) }));
        
        g2.fillRect(0, 0, w, h);
        g2.setColor(ColorUtils.darken(header.getBackground(), 1.1f));
        g2.drawLine(0, h - 1, w, h - 1);
        g2.setColor(ColorUtils.lighten(header.getBackground(), 3.1f));
        g2.drawLine(0, h - 2, w, h - 2);
    }

	@Override
	public void paintLeftLibraryMenu(LeftLibraryMenu control, Graphics2D g2) {
        Taste taste = control.getTaste();
        Color tintColor = taste.getTintColor();
        int w = control.getComponent().getWidth(), h = control.getComponent().getHeight();
        g2.setBackground(tintColor);
        g2.fillRect(0, 0, w, h);
	}

	@Override
	public void paintTree(Tree control, Graphics2D g2) {
        int w = control.getComponent().getWidth(), h = control.getComponent().getHeight();
        Taste taste = control.getTaste();
        Color tintColor = taste.getTintColor();
        if (control.getTaste().isDarkMode()) {
        	tintColor = ColorUtils.darken(tintColor, 0.6f);
        }
        g2.setBackground(tintColor);
        g2.setPaint(tintColor);
        g2.fillRect(0, 0, w, h); 
        System.out.println("Test");
	}

	@Override
	public void paintLeftMenuPanel(LeftMenuPanel control, Graphics2D g2) {
        int w = control.getComponent().getWidth(), h = control.getComponent().getHeight();
        Taste taste = control.getTaste();
        Color tintColor = taste.getTintColor();
    	g2.setBackground(tintColor);
        
        g2.fillRect(0, 0, w, h);
	}

	@Override
	public void paintViewStack(ViewStack control, Graphics2D g2) {
        if (control.getTaste().isDarkMode()) {
            control.getComponent().setBackground(control.getComponent().getBackground().brighter());
        }
	}

    @Override
    public void paintText(JComponent control, Graphics2D g2, String text, int x, int y) {
        Color color = g2.getColor();
        double luminence = ColorUtils.getLuminanceOfColor(color);
        Color bg = luminence > 127 ? Color.BLACK : Color.WHITE;
        g2.setColor(bg);
        g2.drawString(text, x, y);
        g2.setColor(color);
        g2.drawString(text, x, y + 1);
    }

    @Override
    public void paintButton(Button control, Graphics2D g2, boolean hover, boolean pressed) {
        Taste taste = control.getTaste();
        Color tintColor = taste.getTintColor();
        int w = control.getComponent().getWidth(), h = control.getComponent().getHeight();
        System.out.println(control.getComponent().isOpaque());

        if (control.getVariant() == "default") {
            if (control.getComponent().isContentAreaFilled()) {
                g2.setPaint(new LinearGradientPaint(new Point(0, 0), new Point(0, h), new float[] {0, 1}, new Color[] { ColorUtils.darken(tintColor, 1.8f), ColorUtils.darken(tintColor, 1f) }));

                if (pressed) {
                    g2.setPaint(new LinearGradientPaint(new Point(0, 0), new Point(0, h), new float[] {0, 1}, new Color[] { ColorUtils.darken(tintColor, 0.8f), ColorUtils.darken(tintColor, 0.5f) }));
                }
                g2.fillRoundRect(1,  1, w - 2, h - 2, 28, 28);
            }
        }
    }

    @Override
    public void paintFooter(Panel footer, Graphics2D g2) {
        // TODO Auto-generated method stub
        Taste taste = se.spacify.ui.MainWindow.getInstance().getTaste();
        Color tintColor = taste.getTintColor();
        int w = footer.getWidth(), h = footer.getHeight();
        g2.setPaint(new LinearGradientPaint(new Point(0, 0), new Point(0, h), new float[] {0, 1}, new Color[] { ColorUtils.darken(tintColor, 0.8f), ColorUtils.darken(tintColor, 0.5f) }));
      
        g2.fillRect(0, 0, w, h);
        g2.setColor(ColorUtils.darken(footer.getBackground(), 1.1f));
        g2.drawLine(0, 0, w, 0);
        g2.setColor(ColorUtils.lighten(footer.getBackground(), 3.1f));
        g2.drawLine(0, 1, w, 1);
    }

    @Override
    public void paintToolBar(ToolBar control, Graphics2D g2) {
        int w = control.getComponent().getWidth(), h = control.getComponent().getHeight();
        g2.setPaint(new LinearGradientPaint(new Point(0, 0), new Point(0, h), new float[] {0, 1}, new Color[] { ColorUtils.darken(control.getTaste().getTintColor(),1f), ColorUtils.darken(control.getTaste().getTintColor(), 0.9f) }));
        g2.fillRect(0, 0, w, h);
        g2.setColor(ColorUtils.darken(control.getComponent().getBackground(), 1.1f));
        g2.drawLine(0, h - 1, w, h - 1);
        g2.setColor(ColorUtils.lighten(control.getComponent().getBackground(), 7.1f));
        g2.drawLine(0, h - 2, w, h - 2);
    }

	@Override
	public void paintTableHeader(Table jTable, int width, int height, Graphics2D g2) {
        Table table = (Table)jTable;
		g2.setPaint(new LinearGradientPaint(new Point(0, 0), new Point(0, height), new float[] {0, 1}, new Color[] { ColorUtils.darken(table.getTaste().getTintColor(),1f), ColorUtils.darken(table.getTaste().getTintColor(), 0.9f) }));
        g2.fillRect(0, 0, width, height);		
	}

    @Override
    public void paintTextField(TextField control, Graphics2D g2) {
        int w = control.getComponent().getWidth(), h = control.getComponent().getHeight();
 
        g2.setPaint(new LinearGradientPaint(new Point(0, 0), new Point(0, h), new float[] {0, 1}, new Color[] { ColorUtils.darken(control.getComponent().getBackground(),0.3f), ColorUtils.darken(control.getComponent().getBackground(), 0.5f) }));
        g2.fillRect(0, 0, w, h);
    }

	@Override
	public void paintSplitPaneDivider(SplitPaneDivider control, Graphics2D g2) {
		int h = control.getHeight();
		if (ThemeManager.isDarkMode()) {
			g2.setPaint(ColorUtils.darken(control.getBackground(), 0.2f));
		}
        g2.fillRect(0, 0, 1, h);
	}

    @Override
    public String getName() {
        // TODO Auto-generated method stub
        return "Spot 09";
    }
}
