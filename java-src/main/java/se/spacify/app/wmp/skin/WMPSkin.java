package se.spacify.app.wmp.skin;

import java.awt.Color;
import java.awt.Paint;

import javax.swing.JComponent;

import se.spacify.controls.Control;
import se.spacify.skinning.Skin;

public class WMPSkin extends Skin {

	@Override
	public String getName() {
		// TODO Auto-generated method stub
		return "WMP";
	}

	@Override
	public Paint getPaintValue(Control<? extends JComponent> control, String key, Paint defaultValue) {
		if (key == "table.alternateBackground") {
			if (se.spacify.ui.MainWindow.getInstance().getTaste().isDarkMode()) {
				return new Color(255, 255, 255, 25);
			} else {
				return new Color(0, 0, 0, 1);
			}
		}
		return defaultValue;
	}

	@Override
	public Color getColorValue(Control<? extends JComponent> control, String key, Color defaultValue) {
		if (key == "table.alternateBackground") {
			if (se.spacify.ui.MainWindow.getInstance().getTaste().isDarkMode()) {
				return new Color(255, 255, 255, 25);
			} else {
				return new Color(0, 0, 0, 15);
			}
		}
		return defaultValue;
	}

	@Override
	public String getId() {
		// TODO Auto-generated method stub
		return "wmp";
	}

}
