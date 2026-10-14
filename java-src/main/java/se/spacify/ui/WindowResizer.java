package se.spacify.ui;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;

/**
 * Transparent glass-pane that intercepts mouse events within {@code GRIP}
 * pixels of the window edge for resizing, and redispatches everything else
 * to the component below so normal UI interaction is unaffected.
 */
public final class WindowResizer extends JComponent {

    private static final long serialVersionUID = -2686619096872991739L;

	private static final int GRIP = 6;

    // Direction bitmask constants
    private static final int N = 1, S = 2, W = 4, E = 8;

    private final JFrame frame;
    private int          activeDir  = 0;
    private Point        dragOrigin;
    private Rectangle    startBounds;
    // Component that received the current press; drags/release are captured to it
    // so e.g. a JSplitPane divider keeps tracking even when the cursor leaves it.
    private Component    pressTarget;

    public static void install(JFrame frame) {
        WindowResizer r = new WindowResizer(frame);
        frame.setGlassPane(r);
        r.setVisible(true);
    }

    private WindowResizer(JFrame frame) {
        this.frame = frame;
        setOpaque(false);

        addMouseMotionListener(new MouseMotionAdapter() {
            @Override
            public void mouseMoved(MouseEvent e) {
                int dir = dirAt(e.getPoint());
                if (dir != 0) {
                    setCursor(Cursor.getPredefinedCursor(cursorId(dir)));
                } else {
                    // Mirror the cursor of the component below (e.g. the split
                    // divider's resize cursor) so the grip is discoverable.
                    Component c = componentAt(e);
                    setCursor(c != null ? c.getCursor() : Cursor.getDefaultCursor());
                }
            }

            @Override
            public void mouseDragged(MouseEvent e) {
                if (activeDir != 0) doResize(e.getLocationOnScreen());
                else                dispatchTo(pressTarget, e);
            }
        });

        addMouseListener(new MouseAdapter() {
            @Override
            public void mousePressed(MouseEvent e) {
                activeDir = dirAt(e.getPoint());
                if (activeDir != 0) {
                    dragOrigin  = e.getLocationOnScreen();
                    startBounds = frame.getBounds();
                } else {
                    pressTarget = componentAt(e);
                    dispatchTo(pressTarget, e);
                }
            }

            @Override public void mouseReleased(MouseEvent e) {
                if (activeDir == 0) { dispatchTo(pressTarget, e); pressTarget = null; }
                activeDir = 0;
            }
            @Override public void mouseClicked(MouseEvent e)  { if (activeDir == 0) redispatch(e); }
            @Override public void mouseEntered(MouseEvent e)  { redispatch(e); }
            @Override public void mouseExited(MouseEvent e)   { redispatch(e); }
        });
    }

    private void doResize(Point screen) {
        int dx = screen.x - dragOrigin.x;
        int dy = screen.y - dragOrigin.y;
        Rectangle b = new Rectangle(startBounds);

        if ((activeDir & W) != 0) { b.x += dx; b.width  -= dx; }
        if ((activeDir & E) != 0) { b.width  += dx; }
        if ((activeDir & N) != 0) { b.y += dy; b.height -= dy; }
        if ((activeDir & S) != 0) { b.height += dy; }

        Dimension min = frame.getMinimumSize();
        if (b.width  < min.width)  { if ((activeDir & W) != 0) b.x = startBounds.x + startBounds.width  - min.width;  b.width  = min.width; }
        if (b.height < min.height) { if ((activeDir & N) != 0) b.y = startBounds.y + startBounds.height - min.height; b.height = min.height; }

        frame.setBounds(b);
    }

    /** Deepest Swing component under the cursor, or null if none (or this glass pane). */
    private Component componentAt(MouseEvent e) {
        Point p = e.getPoint();
        Component target = SwingUtilities.getDeepestComponentAt(frame.getContentPane(), p.x, p.y);
        return (target == null || target == this) ? null : target;
    }

    /** Forward a mouse event to a specific component, translating coordinates. */
    private void dispatchTo(Component target, MouseEvent e) {
        if (target == null) return;
        Point tp = SwingUtilities.convertPoint(this, e.getPoint(), target);
        target.dispatchEvent(new MouseEvent(target, e.getID(), e.getWhen(),
                e.getModifiersEx(), tp.x, tp.y, e.getClickCount(),
                e.isPopupTrigger(), e.getButton()));
    }

    /** Forward a mouse event to the deepest Swing component under the cursor. */
    private void redispatch(MouseEvent e) {
        dispatchTo(componentAt(e), e);
    }

    private int dirAt(Point p) {
        Dimension sz = getSize();
        int d = 0;
        if (p.x <= GRIP)              d |= W;
        if (p.x >= sz.width  - GRIP)  d |= E;
        if (p.y <= GRIP)              d |= N;
        if (p.y >= sz.height - GRIP)  d |= S;
        return d;
    }

    private static int cursorId(int dir) {
        return switch (dir) {
            case N     -> Cursor.N_RESIZE_CURSOR;
            case S     -> Cursor.S_RESIZE_CURSOR;
            case W     -> Cursor.W_RESIZE_CURSOR;
            case E     -> Cursor.E_RESIZE_CURSOR;
            case N | W -> Cursor.NW_RESIZE_CURSOR;
            case N | E -> Cursor.NE_RESIZE_CURSOR;
            case S | W -> Cursor.SW_RESIZE_CURSOR;
            case S | E -> Cursor.SE_RESIZE_CURSOR;
            default    -> Cursor.DEFAULT_CURSOR;
        };
    }
}
