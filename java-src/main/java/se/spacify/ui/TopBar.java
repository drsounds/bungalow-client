package se.spacify.ui;

import java.awt.Graphics;
import java.awt.Graphics2D;

import se.spacify.controls.Panel;

/**
 * The thin top strip above the app header. A clean {@link Panel} control that
 * paints itself through the active skin's {@code paintTopBar}; children and
 * geometry are configured via {@link #getComponent()}.
 */
public class TopBar extends Panel {

	public TopBar() {
	}

    @Override
    protected void paintSurface(Graphics g) {
        Graphics2D g2 = (Graphics2D) g.create();
        getSkin().paintTopBar(this, g2);
        g2.dispose();
    }
}
