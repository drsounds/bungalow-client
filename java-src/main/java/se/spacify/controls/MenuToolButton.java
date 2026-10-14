package se.spacify.controls;

import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;

import javax.swing.JPopupMenu;
import javax.swing.SwingUtilities;
import javax.swing.event.PopupMenuEvent;
import javax.swing.event.PopupMenuListener;

/**
 * A {@link ToolButton} that pops up a menu (returned by {@link #getPopup()}; add
 * items to it). Clicks outside the menu dismiss it.
 *
 * @see <a href="https://stackoverflow.com/questions/1692677/how-to-create-a-jbutton-with-a-menu">SO 1692677</a>
 */
public class MenuToolButton extends ToolButton
		implements MouseListener, PopupMenuListener {

	private final JPopupMenu popMenu;

	public MenuToolButton() {
		super();
		popMenu = new JPopupMenu();
		component.addMouseListener(this);
		popMenu.addPopupMenuListener(this);
	}

	public JPopupMenu getPopup() { return popMenu; }

	@Override
	public void mousePressed(MouseEvent e) {
		if (!popMenu.isShowing()) {
			popMenu.show(component, 0, component.getBounds().height);
		}
	}

	@Override
	public void popupMenuWillBecomeInvisible(PopupMenuEvent e) {
		SwingUtilities.invokeLater(() -> {
			// If still showing, it was hidden and reshown by a mouse-down in the button.
			if (popMenu.isShowing()) {
				popMenu.setVisible(false);
			}
		});
	}

	@Override public void mouseClicked(MouseEvent e) { }
	@Override public void mouseReleased(MouseEvent e) { }
	@Override public void mouseEntered(MouseEvent e) { }
	@Override public void mouseExited(MouseEvent e) { }
	@Override public void popupMenuWillBecomeVisible(PopupMenuEvent e) { }
	@Override public void popupMenuCanceled(PopupMenuEvent e) { }
}
