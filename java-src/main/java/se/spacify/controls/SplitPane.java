package se.spacify.controls;

import java.awt.Component;
import java.awt.Graphics;
import java.awt.Graphics2D;

import javax.swing.JSplitPane;
import javax.swing.border.Border;
import javax.swing.border.EmptyBorder;
import javax.swing.plaf.basic.BasicSplitPaneDivider;
import javax.swing.plaf.basic.BasicSplitPaneUI;

/**
 * A split-pane container control wrapping a {@link JSplitPane}, with a skinned
 * divider painted via {@link se.spacify.skinning.Skin#paintSplitPaneDivider}.
 * Reach the widget through {@link #getComponent()}.
 */
public class SplitPane extends Control<JSplitPane> {

	public static final int HORIZONTAL_SPLIT = JSplitPane.HORIZONTAL_SPLIT;
	public static final int VERTICAL_SPLIT   = JSplitPane.VERTICAL_SPLIT;

	/** The skinned divider; painted by the active skin. */
	public class SplitPaneDivider extends BasicSplitPaneDivider {
		private static final long serialVersionUID = 1L;
		public SplitPaneDivider(BasicSplitPaneUI ui) {
			super(ui);
			super.setBorder(null);
		}
		@Override public void setBorder(Border border) { /* ignore */ }
		@Override
		public void paint(Graphics g) {
			Graphics2D g2 = (Graphics2D) g.create();
			getTaste().getSkin().paintSplitPaneDivider(this, g2);
			g2.dispose();
		}
	}

	private class SplitPaneDividerUI extends BasicSplitPaneUI {
		@Override
		public BasicSplitPaneDivider createDefaultDivider() {
			return new SplitPaneDivider(this);
		}
	}

	public SplitPane() {
		this.component = new JSplitPane();
		component.setOpaque(false);
		BasicSplitPaneDivider d = getDivider();
		if (d != null) d.setBorder(new EmptyBorder(0, 0, 0, 0));
	}

	/**
	 * @param left  a {@link Control} or a raw {@link Component}
	 * @param right a {@link Control} or a raw {@link Component}
	 */
	public SplitPane(int orientation, Object left, Object right) {
		this.component = new JSplitPane(orientation, comp(left), comp(right));
		component.setUI(new SplitPaneDividerUI());
		BasicSplitPaneDivider d = getDivider();
		if (d != null) d.setBorder(new EmptyBorder(0, 0, 0, 0));
		if (left instanceof Control<?> l)  { children.add(l); l.setParent(this); }
		if (right instanceof Control<?> r) { children.add(r); r.setParent(this); }
	}

	private static Component comp(Object o) {
		return o instanceof Control<?> c ? c.getComponent() : (Component) o;
	}

	public BasicSplitPaneDivider getDivider() {
		if (component.getUI() instanceof BasicSplitPaneUI ui) {
			return ui.getDivider();
		}
		return null;
	}
}
