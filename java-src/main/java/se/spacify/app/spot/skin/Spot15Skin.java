package se.spacify.app.spot.skin;

import java.awt.Color;
import java.awt.Graphics2D;

import se.spacify.controls.SplitPane.SplitPaneDivider;

import se.spacify.controls.TextField;

import se.spacify.navigation.ViewStack;

import se.spacify.ui.theme.ThemeManager;

public class Spot15Skin extends SpotSkin {

    @Override
    public String getId() {
        // TODO Auto-generated method stub
        return "spot";
    }

	@Override
	public void paintViewStack(ViewStack control, Graphics2D g2) {
        if (control.getTaste().isDarkMode()) {
            control.getComponent().setBackground(control.getComponent().getBackground().brighter());
        }
	}

	@Override
	public void paintSplitPaneDivider(SplitPaneDivider control, Graphics2D g2) {
		int w = control.getWidth(), h = control.getHeight();
		if (ThemeManager.isDarkMode()) {
			g2.setPaint(control.getBackground().darker());
			g2.fillRect(0, 0, w, h);
		}
	}

    @Override
    public void paintTextField(TextField control, Graphics2D g2) {

        Color bgColor = new Color(255, 255, 255, 255);
        Color fgColor = new Color(0, 0, 0, 0);

        if (control.getMainWindow().getTaste().isDarkMode()) {
            bgColor = new Color(0, 0, 0, 255);
            fgColor = new Color(0, 0, 0, 255);
        }
 
        control.getComponent().setForeground(fgColor);
        control.getComponent().setBackground(bgColor);
    }

    @Override
    public String getName() {
        // TODO Auto-generated method stub
        return "Spot";
    }
}
