package se.spacify.controls;

import java.awt.Graphics;

import javax.swing.JTable;
import javax.swing.table.TableModel;

import se.spacify.ui.theme.ThemeManager;
import se.spacify.ui.theme.ThemedTableCellRenderer;

/**
 * A table control wrapping a {@link JTable} with Spacify's row height and the
 * skinned header/cell renderers installed. Reach the widget through
 * {@link #getComponent()}.
 */
public class Table extends Control<JTable> {

	/** Row height in pixels, matching the Windows ListView default. */
	public static final int ROW_HEIGHT = 24;

	public Table() {
		this.component = new ZebraTable();
		init();
	}

	public Table(Object[][] data, String[] columns) {
		this.component = new ZebraTable(data, columns);
		init();
	}

	public Table(TableModel model) {
		this.component = new ZebraTable(model);
		init();
	}

	private void init() {
		component.setRowHeight(ROW_HEIGHT);
		component.getTableHeader().setDefaultRenderer(new TableHeaderRenderer(this));
		component.setDefaultRenderer(Table.class, new SpaceTableCellRenderer(this));
	}

	/**
	 * A {@link JTable} that, when {@link ThemeManager#isFillEmptyRows()} is on,
	 * keeps painting the zebra striping in the blank area below the last row so a
	 * short list still fills the viewport with stripes instead of flat background.
	 */
	private static final class ZebraTable extends JTable {
		private static final long serialVersionUID = 1L;

		ZebraTable() { super(); }
		ZebraTable(Object[][] data, String[] columns) { super(data, columns); }
		ZebraTable(TableModel model) { super(model); }

		@Override
		protected void paintComponent(Graphics g) {
			super.paintComponent(g);
			if (!ThemeManager.isFillEmptyRows())
				return;
			int rowH = getRowHeight();
			if (rowH <= 0)
				return;
			int rows = getRowCount();
			int top  = rows == 0 ? 0 : getCellRect(rows - 1, 0, true).y + rowH;
			int h = getHeight(), w = getWidth();
			for (int y = top, r = rows; y < h; y += rowH, r++) {
				g.setColor(ThemedTableCellRenderer.rowBackground(this, r));
				g.fillRect(0, y, w, rowH);
			}
		}
	}
}
