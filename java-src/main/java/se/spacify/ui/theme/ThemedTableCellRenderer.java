package se.spacify.ui.theme;

import java.awt.Color;
import java.awt.Component;

import javax.swing.BorderFactory;
import javax.swing.JTable;
import javax.swing.table.DefaultTableCellRenderer;

import se.spacify.skinning.Skin;
import se.spacify.ui.MainWindow;

// ── Custom renderer ───────────────────────────────────────────────────────

public class ThemedTableCellRenderer extends DefaultTableCellRenderer {
    private static final long serialVersionUID = 6139658182597024812L;

    @Override
    public Component getTableCellRendererComponent(JTable table, Object value,
            boolean isSelected, boolean hasFocus, int row, int column) {
        MainWindow mw = MainWindow.getInstance();
        Taste taste = mw.getTaste();
        super.getTableCellRendererComponent(table, value, isSelected, hasFocus, row, column);

        if (isSelected) {
            setBackground(taste.getAccentBackgroundColor());
            setForeground(taste.getAccentForegroundColor());
        } else {
            setBackground(rowBackground(table, row));
            setForeground(table.getForeground());
        }
        setBorder(BorderFactory.createEmptyBorder(0, 6, 0, 6));
        return this;
    }

    /**
     * The zebra background for a (zero-based) table row: the skin's alternate-row
     * shade on even rows, the table background on odd rows. Shared so cells that
     * paint their own content (e.g. the Buy/Stream button) and the empty area below
     * the last row can match the striping exactly.
     */
    public static Color rowBackground(JTable table, int row) {
        Skin skin = MainWindow.getInstance().getTaste().getSkin();
        // This renderer holds only the raw JTable, not its Control; the skin's
        // colour lookup keys off the taste, not the control, so null is fine here.
        return row % 2 == 0
            ? skin.getColorValue(null, "table.alternateBackground", new Color(0, 0, 0, 11))
            : table.getBackground();
    }
}
