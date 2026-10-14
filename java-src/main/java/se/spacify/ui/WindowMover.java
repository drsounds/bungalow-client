package se.spacify.ui;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;

/**
 * Installs drag-to-move behaviour on a header component.
 * Recursively attaches to non-interactive descendants (panels, labels)
 * so the user can drag from any blank area inside the header.
 * Interactive controls (buttons, text fields, sliders) are skipped.
 */
public final class WindowMover extends MouseAdapter {

    private Point dragOrigin;

    private WindowMover() {}

    public static void install(JComponent header) {
        WindowMover m = new WindowMover();
        attach(header, m);
    }

    private static void attach(Component c, WindowMover m) {
        if (c instanceof AbstractButton || c instanceof JTextField ||
                c instanceof JSlider   || c instanceof JComboBox<?>)
            return;
        c.addMouseListener(m);
        c.addMouseMotionListener(m);
        if (c instanceof Container ct)
            for (Component child : ct.getComponents()) attach(child, m);
    }

    @Override
    public void mousePressed(MouseEvent e) {
        dragOrigin = e.getLocationOnScreen();
    }

    @Override
    public void mouseDragged(MouseEvent e) {
        if (dragOrigin == null) return;
        Window w = SwingUtilities.getWindowAncestor(e.getComponent());
        if (w == null) return;
        Point cur = e.getLocationOnScreen();
        w.setLocation(w.getX() + cur.x - dragOrigin.x, w.getY() + cur.y - dragOrigin.y);
        dragOrigin = cur;
    }

    @Override
    public void mouseReleased(MouseEvent e) { dragOrigin = null; }
}
