package se.spacify.app.ui;

import se.spacify.app.ApplicationManager.ManagedApplication;
import se.spacify.app.ApplicationSetting;
import se.spacify.app.ApplicationSettings;
import se.spacify.ui.theme.ThemeManager;

import javax.swing.*;
import java.awt.*;
import java.util.ArrayList;
import java.util.List;

/**
 * Builds an editor for a plugin's {@link ApplicationSetting} schema — one control per
 * type (checkbox / text field / spinner / multi-line list) — backed by the
 * plugin's {@link ApplicationSettings}. "Save" writes every control back and persists.
 */
class ApplicationSettingsEditor extends JPanel {

    private static final long serialVersionUID = 4892696129856064121L;
	private final ApplicationSettings settings;
    private final List<Runnable> writers = new ArrayList<>();

    ApplicationSettingsEditor(ManagedApplication plugin) {
        this.settings = plugin.getSettings();
        setOpaque(false);
        setLayout(new GridBagLayout());
        GridBagConstraints c = new GridBagConstraints();
        c.insets = new Insets(4, 6, 4, 6);
        c.anchor = GridBagConstraints.WEST;

        Color fg = ThemeManager.getForeground();
        int row = 0;
        List<ApplicationSetting> schema = plugin.getSchema();
        if (schema.isEmpty()) {
            JLabel none = new JLabel("This plugin has no settings.");
            none.setForeground(fg);
            add(none, gbc(c, 0, 0, 2));
        }
        for (ApplicationSetting s : schema) {
            JLabel label = new JLabel(s.getLabel());
            label.setForeground(fg);
            add(label, gbc(c, 0, row, 1));
            add(editorFor(s), gbc(c, 1, row, 1));
            row++;
        }

        JButton save = new JButton("Save");
        save.addActionListener(e -> save());
        c.gridx = 1; c.gridy = row; c.weightx = 0; c.fill = GridBagConstraints.NONE;
        add(save, c);
    }

    private Component editorFor(ApplicationSetting s) {
        String key = s.getKey();
        switch (s.getType()) {
            case BOOL -> {
                JCheckBox cb = new JCheckBox();
                cb.setOpaque(false);
                cb.setSelected(settings.getBool(key));
                writers.add(() -> settings.set(key, cb.isSelected()));
                return cb;
            }
            case STRING -> {
                JTextField tf = new JTextField(settings.getString(key), 20);
                writers.add(() -> settings.set(key, tf.getText()));
                return tf;
            }
            case INT -> {
                JSpinner sp = new JSpinner(new SpinnerNumberModel(settings.getInt(key),
                    Integer.MIN_VALUE, Integer.MAX_VALUE, 1));
                sp.setPreferredSize(new Dimension(100, 24));
                writers.add(() -> settings.set(key, ((Number) sp.getValue()).intValue()));
                return sp;
            }
            case STRING_ARRAY -> {
                JTextArea ta = multiline(String.join("\n", settings.getStringArray(key)));
                writers.add(() -> settings.set(key, splitNonEmpty(ta.getText())));
                return new JScrollPane(ta);
            }
            case INT_ARRAY -> {
                int[] vals = settings.getIntArray(key);
                StringBuilder sb = new StringBuilder();
                for (int i = 0; i < vals.length; i++) { if (i > 0) sb.append('\n'); sb.append(vals[i]); }
                JTextArea ta = multiline(sb.toString());
                writers.add(() -> settings.set(key, parseInts(ta.getText())));
                return new JScrollPane(ta);
            }
            default -> { return new JLabel(); }
        }
    }

    private void save() {
        for (Runnable w : writers) w.run();
        settings.save();
        JOptionPane.showMessageDialog(this, "Settings saved.", "Application Settings",
            JOptionPane.INFORMATION_MESSAGE);
    }

    private static JTextArea multiline(String text) {
        JTextArea ta = new JTextArea(text, 3, 20);
        ta.setToolTipText("One value per line");
        return ta;
    }

    private static String[] splitNonEmpty(String text) {
        List<String> out = new ArrayList<>();
        for (String line : text.split("\n", -1)) if (!line.isBlank()) out.add(line.trim());
        return out.toArray(new String[0]);
    }

    private static int[] parseInts(String text) {
        List<Integer> out = new ArrayList<>();
        for (String line : text.split("\n", -1)) {
            String t = line.trim();
            if (t.isEmpty()) continue;
            try { out.add(Integer.parseInt(t)); } catch (NumberFormatException ignored) {}
        }
        int[] r = new int[out.size()];
        for (int i = 0; i < r.length; i++) r[i] = out.get(i);
        return r;
    }

    private static GridBagConstraints gbc(GridBagConstraints c, int x, int y, int w) {
        c.gridx = x; c.gridy = y; c.gridwidth = w;
        c.weightx = (x == 1) ? 1 : 0;
        c.fill = (x == 1) ? GridBagConstraints.HORIZONTAL : GridBagConstraints.NONE;
        return c;
    }
}
