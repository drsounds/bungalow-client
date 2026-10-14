package se.spacify.app.ui;

import se.spacify.controls.Table;
import se.spacify.navigation.View;
import se.spacify.navigation.ViewStack;
import se.spacify.app.ApplicationDescriptor;

import se.spacify.app.ApplicationManager.ManagedApplication;
import se.spacify.ui.theme.ThemeManager;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * Application manager, reachable at {@code spacify:apps}. Lists every discovered
 * plugin with an enable/disable checkbox, supports installing external jars
 * (copied into {@code ~/Bungalow}) and removing them, and shows an
 * auto-generated settings editor for the selected plugin.
 */
public class ApplicationManagerView extends View {

    private final javax.swing.JTable table;
    private final DefaultTableModel model;
    private final JScrollPane      detail;
    private final List<ManagedApplication> rows = new ArrayList<>();

    private boolean refreshing = false;

    @SuppressWarnings("static-access")
    public ApplicationManagerView(ViewStack viewStack) {
        super(viewStack);
        getComponent().setLayout(new BorderLayout(0, 8));
        getComponent().setOpaque(true);
        getComponent().setBorder(BorderFactory.createEmptyBorder(16, 16, 16, 16));

        // ── Header + toolbar ─────────────────────────────────────────────────
        JLabel title = new JLabel("Apps");
        title.setFont(title.getFont().deriveFont(Font.BOLD, 20f));

        JButton addBtn    = new JButton("Add…");
        JButton removeBtn = new JButton("Remove");
        addBtn.addActionListener(e -> onAdd());
        removeBtn.addActionListener(e -> onRemove());

        JPanel buttons = new JPanel(new FlowLayout(FlowLayout.RIGHT, 6, 0));
        buttons.setOpaque(false);
        buttons.add(addBtn);
        buttons.add(removeBtn);

        JPanel header = new JPanel(new BorderLayout());
        header.setOpaque(false);
        header.add(title, BorderLayout.WEST);
        header.add(buttons, BorderLayout.EAST);
        getComponent().add(header, BorderLayout.NORTH);

        // ── Application table ─────────────────────────────────────────────────────
        model = new DefaultTableModel(new String[]{"On", "Name", "Version", "Source"}, 0) {
            @Override public boolean isCellEditable(int r, int c) { return c == 0; }
            @Override public Class<?> getColumnClass(int c) { return c == 0 ? Boolean.class : String.class; }
        };
        table = new Table(model).getComponent();
        table.setShowGrid(false);
        table.getColumnModel().getColumn(0).setMaxWidth(32);
        table.getSelectionModel().addListSelectionListener(e -> { if (!e.getValueIsAdjusting()) showSelected(); });
        model.addTableModelListener(e -> {
            if (refreshing || e.getColumn() != 0) return;
            int r = e.getFirstRow();
            if (r >= 0 && r < rows.size()) {
                boolean on = Boolean.TRUE.equals(model.getValueAt(r, 0));
                getViewStack().getMainWindow().getApplicationManager().setEnabled(rows.get(r).getDescriptor().getId(), on);
            }
        });

        JScrollPane tableScroll = new JScrollPane(table);
        tableScroll.setBorder(BorderFactory.createEmptyBorder());

        detail = new JScrollPane();
        detail.setBorder(BorderFactory.createEmptyBorder());

        JSplitPane split = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, tableScroll, detail);
        split.setDividerLocation(360);
        split.setBorder(BorderFactory.createEmptyBorder());
        split.setOpaque(false);
        getComponent().add(split, BorderLayout.CENTER);

        refresh();
        applyTheme();
        getViewStack().getMainWindow().getApplicationManager().addChangeListener(() -> SwingUtilities.invokeLater(this::refresh));
        getViewStack().getMainWindow().getThemeManager().addChangeListener(this::applyTheme);
    }

    // ── Actions ──────────────────────────────────────────────────────────────

    private void onAdd() {
        JFileChooser chooser = new JFileChooser();
        chooser.setFileFilter(new javax.swing.filechooser.FileNameExtensionFilter("Application jar (*.jar)", "jar"));
        if (chooser.showOpenDialog(getComponent()) != JFileChooser.APPROVE_OPTION) return;
        File jar = chooser.getSelectedFile();
        boolean ok = getViewStack().getMainWindow().getApplicationManager().install(jar);
        if (!ok) {
            JOptionPane.showMessageDialog(getComponent(),
                "Not a valid Spacify plugin jar (missing Spacify-App-Id / -Class manifest headers).",
                "Install failed", JOptionPane.ERROR_MESSAGE);
        }
    }

    private void onRemove() {
        ManagedApplication m = selected();
        if (m == null) return;
        if (!m.getDescriptor().isRemovable()) {
            JOptionPane.showMessageDialog(getComponent(),
                "Built-in plugins can't be removed — disable it with the checkbox instead.",
                "Remove", JOptionPane.INFORMATION_MESSAGE);
            return;
        }
        int ans = JOptionPane.showConfirmDialog(getComponent(),
            "Remove plugin \"" + m.getDescriptor().getName() + "\"?",
            "Remove plugin", JOptionPane.YES_NO_OPTION, JOptionPane.WARNING_MESSAGE);
        if (ans == JOptionPane.YES_OPTION) {
            getViewStack().getMainWindow().getApplicationManager().uninstall(m.getDescriptor().getId());
        }
    }

    // ── Table / detail rendering ─────────────────────────────────────────────

    private void refresh() {
        String selectedId = selected() != null ? selected().getDescriptor().getId() : null;

        refreshing = true;
        rows.clear();
        model.setRowCount(0);
        for (ManagedApplication m : getViewStack().getMainWindow().getApplicationManager().getApplications()) {
            rows.add(m);
            ApplicationDescriptor d = m.getDescriptor();
            model.addRow(new Object[]{ m.isEnabled(), d.getName(), d.getVersion(), sourceLabel(d.getSource()) });
        }
        refreshing = false;

        // Restore selection by id.
        int sel = -1;
        for (int i = 0; i < rows.size(); i++)
            if (rows.get(i).getDescriptor().getId().equals(selectedId)) { sel = i; break; }
        if (sel < 0 && !rows.isEmpty()) sel = 0;
        if (sel >= 0) table.setRowSelectionInterval(sel, sel);
        else detail.setViewportView(null);
        applyTheme();
    }

    @SuppressWarnings("static-access")
    private void showSelected() {
        ManagedApplication m = selected();
        if (m == null) { detail.setViewportView(null); return; }

        JPanel info = new JPanel(new BorderLayout(0, 8));
        info.setOpaque(false);
        info.setBorder(BorderFactory.createEmptyBorder(4, 12, 4, 4));

        ApplicationDescriptor d = m.getDescriptor();
        JLabel head = new JLabel("<html><b>" + d.getName() + "</b> " + d.getVersion()
            + "<br><span style='font-size:9px'>" + d.getId() + " — "
            + sourceLabel(d.getSource()) + (m.isActive() ? " — active" : "") + "</span></html>");
        head.setForeground(getViewStack().getMainWindow().getThemeManager().getForeground());
        if (m.getIcon() != null) head.setIcon(m.getIcon());
        info.add(head, BorderLayout.NORTH);
        info.add(new ApplicationSettingsEditor(m), BorderLayout.CENTER);

        detail.setViewportView(info);
    }

    private ManagedApplication selected() {
        int r = table.getSelectedRow();
        return (r >= 0 && r < rows.size()) ? rows.get(r) : null;
    }

    private static String sourceLabel(ApplicationDescriptor.Source s) {
        return switch (s) {
            case BUILTIN_BUNDLE -> "built-in";
            case APP_DIR        -> "bundled";
            case EXTERNAL       -> "external";
        };
    }

    private void applyTheme() {
        Color bg = ThemeManager.getBackground();
        Color fg = ThemeManager.getForeground();
        getComponent().setBackground(bg);
        table.setBackground(bg);
        table.setForeground(fg);
        table.setGridColor(ThemeManager.getGridColor());
        table.setSelectionBackground(ThemeManager.getAccentForegroundColor());
        table.setSelectionForeground(Color.WHITE);
        detail.getViewport().setBackground(bg);
        detail.setBackground(bg);
        repaint();
    }

    // ── SPView ───────────────────────────────────────────────────────────────

    @Override public boolean acceptsUri(String uri) { return "spacify:apps".equals(uri); }
    @Override public void navigate(String uri) {}
    @Override public String getName() { return "Apps"; }
    @Override public void onShow() { refresh(); }
}
