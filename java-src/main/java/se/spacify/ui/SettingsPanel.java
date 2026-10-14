package se.spacify.ui;

import se.spacify.design.Design;
import se.spacify.skinning.Skin;
import se.spacify.controls.Panel;

import javax.swing.*;
import javax.swing.border.TitledBorder;
import java.awt.*;

public class SettingsPanel extends Panel {
	private static final Color BG   = Color.BLACK;
    private static final Color FG   = Color.WHITE;
    private static final Color FG_DIM = new Color(180, 180, 180);
    private Skin[] skins;

    public SettingsPanel() {
        getComponent().setLayout(new BorderLayout(12, 0));
        getComponent().setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createMatteBorder(1, 0, 0, 0, new Color(50, 50, 50)),
            BorderFactory.createEmptyBorder(8, 16, 8, 16)
        ));
        getComponent().setPreferredSize(new Dimension(0, 150));

        // ── Background tint sliders ──────────────────────────────────────────
        JPanel tintSection = new JPanel(new GridBagLayout()) {
            @Override public void updateUI() { super.updateUI(); setOpaque(false); }
        };
        tintSection.setOpaque(false);
        tintSection.setBorder(titledBorder("Background Tint"));

        GridBagConstraints c = new GridBagConstraints();
        c.insets = new Insets(1, 5, 1, 5);

        JSlider hueSlider   = mkSlider(0, 360, (int)(getTaste().getHue()        * 360));
        JSlider satSlider   = mkSlider(0, 100, (int)(getTaste().getSaturation() * 100));
        JSlider lightSlider = mkSlider(0, 100, (int)(getTaste().getLightness()  * 100));

        addRow(tintSection, c, 0, "Hue",        hueSlider);
        addRow(tintSection, c, 1, "Saturation", satSlider);
        addRow(tintSection, c, 2, "Lightness",  lightSlider);

        // ── Design selector ───────────────────────────────────────────────────
        // A Design is a premix of Chrome + Skin contributed by an installed plugin
        // (WMP, Spot, …); selecting one rebuilds the window in that look and feel.
        JPanel designSection = new JPanel() {
            @Override public void updateUI() { super.updateUI(); setOpaque(false); }
        };
        designSection.setLayout(new BoxLayout(designSection, BoxLayout.Y_AXIS));
        designSection.setOpaque(false);
        designSection.setBorder(titledBorder("Design"));

        Design[] designs = getMainWindow().getDesignManager().all().toArray(new Design[0]);
        JComboBox<Design> designCombo = new JComboBox<>(designs);
        designCombo.setMaximumSize(new Dimension(200, 24));
        designCombo.setAlignmentX(Component.LEFT_ALIGNMENT);
        designCombo.setRenderer(new DefaultListCellRenderer() {
            @Override public Component getListCellRendererComponent(JList<?> list, Object value,
                    int index, boolean sel, boolean focus) {
                super.getListCellRendererComponent(list, value, index, sel, focus);
                if (value instanceof Design d) setText(d.getName());
                return this;
            }
        });
        // Select the active Design first, then attach the listener so wiring up the
        // combo doesn't trigger a (recursive) rebuild during start-up.
        designCombo.setSelectedItem(getMainWindow().getTaste().getDesign());
        designCombo.addActionListener(e -> {
            Design sel = (Design) designCombo.getSelectedItem();
            if (sel != null) getMainWindow().setDesign(sel);
        });
        designSection.add(Box.createVerticalGlue());
        designSection.add(designCombo);
        designSection.add(Box.createVerticalGlue());

        // ── Skin selector ─────────────────────────────────────────────────────
        JPanel skinSection = new JPanel() {
            @Override public void updateUI() { super.updateUI(); setOpaque(false); }
        };
        skinSection.setLayout(new BoxLayout(skinSection, BoxLayout.Y_AXIS));
        skinSection.setOpaque(false);
        skinSection.setBorder(titledBorder("Skin"));

        skins = getMainWindow().getSkinManager().getSkins(Skin.class).toArray(new Skin[0]);

        JComboBox<Skin> skinCombo = new JComboBox<>();
        skinCombo.setMaximumSize(new Dimension(200, 24));
        skinCombo.setAlignmentX(Component.LEFT_ALIGNMENT);
        for (Skin item : skins) {
            if (item.equals(getMainWindow().getSkin())) {
                skinCombo.setSelectedItem(item);
                break;
            }
        }
        skinCombo.addActionListener(e -> {
            Skin sel = (Skin) skinCombo.getSelectedItem();
            if (sel != null) getMainWindow().setSkin(sel);
        });
        skinSection.add(Box.createVerticalGlue());
        skinSection.add(skinCombo);
        skinSection.add(Box.createVerticalGlue());

        // ── Display options (toggles) ─────────────────────────────────────────
        JPanel optionsSection = new JPanel() {
            @Override public void updateUI() { super.updateUI(); setOpaque(false); }
        };
        optionsSection.setLayout(new BoxLayout(optionsSection, BoxLayout.Y_AXIS));
        optionsSection.setOpaque(false);
        optionsSection.setBorder(titledBorder("Display"));

        JCheckBox stripedBox   = whiteCheck("Striped rows",        getTaste().isStripedRows());
        JCheckBox fillRowsBox  = whiteCheck("Fill empty rows",     getTaste().isFillEmptyRows());
        JCheckBox contrastBox  = whiteCheck("B/W background",       getTaste().isHighContrast());
        JCheckBox invertedBox  = whiteCheck("Inverted (dark)",      getTaste().isHighContrastInverted());
        JCheckBox tintTextBox  = whiteCheck("Tint text (light)",    getTaste().isTintText());

        invertedBox.setEnabled(getTaste().isHighContrast());

        stripedBox.addActionListener(e  -> getTaste().setStripedRows(stripedBox.isSelected()));
        fillRowsBox.addActionListener(e -> getTaste().setFillEmptyRows(fillRowsBox.isSelected()));
        contrastBox.addActionListener(e -> {
            getTaste().setHighContrast(contrastBox.isSelected());
            invertedBox.setEnabled(contrastBox.isSelected());
        });
        invertedBox.addActionListener(e -> getTaste().setHighContrastInverted(invertedBox.isSelected()));
        tintTextBox.addActionListener(e -> getTaste().setTintText(tintTextBox.isSelected()));

        optionsSection.add(stripedBox);
        optionsSection.add(fillRowsBox);
        optionsSection.add(contrastBox);
        optionsSection.add(invertedBox);
        optionsSection.add(tintTextBox);

        // ── Mode (dark / light) ──────────────────────────────────────────────
        JPanel modeSection = new JPanel() {
            @Override public void updateUI() { super.updateUI(); setOpaque(false); }
        };
        modeSection.setLayout(new BoxLayout(modeSection, BoxLayout.Y_AXIS));
        modeSection.setOpaque(false);
        modeSection.setBorder(titledBorder("Mode"));

        JRadioButton darkBtn  = whiteRadio("Dark",  getTaste().isDarkMode());
        JRadioButton lightBtn = whiteRadio("Light", !getTaste().isDarkMode());
        ButtonGroup modeGroup = new ButtonGroup();
        modeGroup.add(darkBtn);
        modeGroup.add(lightBtn);
        modeSection.add(darkBtn);
        modeSection.add(Box.createVerticalStrut(2));
        modeSection.add(lightBtn);

        // ── Accent color picker ──────────────────────────────────────────────
        JPanel accentSection = new JPanel() {
            @Override public void updateUI() { super.updateUI(); setOpaque(false); }
        };
        accentSection.setLayout(new BoxLayout(accentSection, BoxLayout.Y_AXIS));
        accentSection.setOpaque(false);
        accentSection.setBorder(titledBorder("Accent Color"));

        JButton accentBtn = new JButton();
        accentBtn.setBackground(getTaste().getAccentBackgroundColor());
        accentBtn.setPreferredSize(new Dimension(48, 48));
        accentBtn.setMaximumSize(new Dimension(48, 48));
        accentBtn.setAlignmentX(Component.CENTER_ALIGNMENT);
        accentBtn.setFocusPainted(false);
        accentBtn.setToolTipText("Click to choose accent color");
        accentBtn.addActionListener(e -> {
            Color chosen = JColorChooser.showDialog(
                SwingUtilities.getWindowAncestor(getComponent()), "Accent Color",
                getTaste().getAccentBackgroundColor());
            if (chosen != null) {
                getTaste().setAccentBackgroundColor(chosen);
                accentBtn.setBackground(chosen);
            }
        });
        accentSection.add(Box.createVerticalGlue());
        accentSection.add(accentBtn);
        accentSection.add(Box.createVerticalGlue());

        // ── Listeners ────────────────────────────────────────────────────────
        hueSlider.addChangeListener(e -> getTaste().setHue(hueSlider.getValue() / 360f));
        satSlider.addChangeListener(e -> getTaste().setSaturation(satSlider.getValue() / 100f));
        lightSlider.addChangeListener(e -> getTaste().setLightness(lightSlider.getValue() / 100f));
        darkBtn.addActionListener(e -> getTaste().setDarkMode(true));
        lightBtn.addActionListener(e -> getTaste().setDarkMode(false));

        JPanel right = new JPanel() {
            @Override public void updateUI() { super.updateUI(); setOpaque(false); }
        };
        right.setLayout(new BoxLayout(right, BoxLayout.X_AXIS));
        right.setOpaque(false);
        right.add(designSection);
        right.add(Box.createHorizontalStrut(8));
        right.add(skinSection);
        right.add(Box.createHorizontalStrut(8));
        right.add(optionsSection);
        right.add(Box.createHorizontalStrut(8));
        right.add(modeSection);
        right.add(Box.createHorizontalStrut(8));
        right.add(accentSection);

        getComponent().add(tintSection, BorderLayout.CENTER);
        getComponent().add(right, BorderLayout.EAST);
    }

    @Override
    protected void paintSurface(Graphics g) {
        g.setColor(BG);
        g.fillRect(0, 0, getComponent().getWidth(), getComponent().getHeight());
    }

    // ── Helpers ──────────────────────────────────────────────────────────────

    private static JSlider mkSlider(int min, int max, int val) {
        JSlider s = new JSlider(min, max, val);
        s.setOpaque(false);
        return s;
    }

    private static void addRow(JPanel panel, GridBagConstraints c, int row,
                               String text, JSlider slider) {
        c.gridx = 0; c.gridy = row; c.weightx = 0;
        c.fill = GridBagConstraints.NONE;
        c.anchor = GridBagConstraints.WEST;
        JLabel lbl = whiteLabel(text);
        lbl.setPreferredSize(new Dimension(68, 20));
        panel.add(lbl, c);

        c.gridx = 1; c.weightx = 1;
        c.fill = GridBagConstraints.HORIZONTAL;
        panel.add(slider, c);
    }

    private static JLabel whiteLabel(String text) {
        return new JLabel(text) {
            @Override public void updateUI() {
                super.updateUI();
                setForeground(FG);
            }
        };
    }

    private static JRadioButton whiteRadio(String text, boolean selected) {
        return new JRadioButton(text, selected) {
            @Override public void updateUI() {
                super.updateUI();
                setForeground(FG);
                setOpaque(false);
            }
        };
    }

    private static JCheckBox whiteCheck(String text, boolean selected) {
        JCheckBox cb = new JCheckBox(text, selected) {
            @Override public void updateUI() {
                super.updateUI();
                setForeground(FG);
                setOpaque(false);
            }
        };
        cb.setFont(cb.getFont().deriveFont(11f));
        cb.setAlignmentX(Component.LEFT_ALIGNMENT);
        return cb;
    }

    // ── Skin catalogue ─────────────────────────────────────────────────────────
 
    private static TitledBorder titledBorder(String title) {
        TitledBorder b = BorderFactory.createTitledBorder(
            BorderFactory.createEtchedBorder(), title);
        b.setTitleFont(b.getTitleFont().deriveFont(10f));
        b.setTitleColor(FG_DIM);
        return b;
    }
}
