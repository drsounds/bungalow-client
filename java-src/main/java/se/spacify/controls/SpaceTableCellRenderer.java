package se.spacify.controls;

import java.awt.Component;
import java.awt.FontMetrics;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.Dimension;
import java.awt.Insets;
import java.awt.Rectangle;

import javax.swing.JComponent;
import javax.swing.JLabel;
import javax.swing.JTable;
import javax.swing.SwingUtilities;
import javax.swing.plaf.LabelUI;
import javax.swing.table.TableCellRenderer;

/**
 * Table cell renderer rendering through the active skin. As a {@link Label}
 * control its returned component is the wrapped {@link JLabel}
 * ({@link #getComponent()}).
 */
public class SpaceTableCellRenderer extends Label implements TableCellRenderer {

	private final Table table;

	public SpaceTableCellRenderer(Table table) {
		super();
		this.table = table;
	}

	public Table getTable() {
		return table;
	}

	private class SpaceTableCellLabelUI extends LabelUI {

		private final Insets PADS = new Insets(4, 4, 4, 4);

		@Override
		public Dimension getPreferredSize(JComponent c) {
			JLabel b = (JLabel) c;
			FontMetrics fm = b.getFontMetrics(b.getFont());
			Rectangle viewR = new Rectangle();
			Rectangle iconR = new Rectangle();
			Rectangle textR = new Rectangle();
			SwingUtilities.layoutCompoundLabel(c, fm, b.getText(), b.getIcon(),
				b.getVerticalAlignment(), b.getHorizontalAlignment(),
				b.getVerticalTextPosition(), b.getHorizontalTextPosition(),
				viewR, iconR, textR, b.getText() == null ? 0 : b.getIconTextGap());
			Rectangle totalBounds = iconR.union(textR);
			Insets insets = b.getInsets();
			int width = totalBounds.width + PADS.left + PADS.right + insets.left + insets.right;
			int height = totalBounds.height + PADS.top + PADS.bottom + insets.top + insets.bottom;
			return new Dimension(width, height);
		}

		@Override
		public void paint(Graphics g, JComponent c) {
			JLabel l = (JLabel) c;
			Graphics2D g2 = (Graphics2D) g.create();
			g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
			FontMetrics fm = g2.getFontMetrics(l.getFont());
			Insets insets = l.getInsets();
			Rectangle viewR = new Rectangle(insets.left, insets.top,
								c.getWidth() - (insets.left + insets.right),
								c.getHeight() - (insets.top + insets.bottom));
			Rectangle iconR = new Rectangle();
			Rectangle textR = new Rectangle();
			String text = SwingUtilities.layoutCompoundLabel(c, fm, l.getText(), l.getIcon(),
				l.getVerticalAlignment(), l.getHorizontalAlignment(),
				l.getVerticalTextPosition(), l.getHorizontalTextPosition(),
				viewR, iconR, textR, l.getText() == null ? 0 : l.getIconTextGap());
			if (l.getIcon() != null) {
				l.getIcon().paintIcon(c, g2, iconR.x, iconR.y);
			}
			if (text != null && !text.isEmpty()) {
				getSkin().paintText(l, g2, text, textR.x, textR.y + fm.getAscent());
			}
			g2.dispose();
		}
	}

	@Override
	public Component getTableCellRendererComponent(JTable table, Object value,
			boolean isSelected, boolean hasFocus, int row, int column) {
		getComponent().setText(value == null ? "" : value.toString());
		getComponent().setUI(new SpaceTableCellLabelUI());
		return getComponent();
	}
}
