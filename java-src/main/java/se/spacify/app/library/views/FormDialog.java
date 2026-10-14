package se.spacify.app.library.views;

import javax.swing.*;
import java.awt.*;

/** Tiny helper that lays out labelled fields and shows them in an OK/Cancel modal. */
final class FormDialog {

    private FormDialog() {}

    /** @return true if the user pressed OK. */
    static boolean show(Component parent, String title, String[] labels, JComponent[] fields) {
        JPanel form = new JPanel(new GridBagLayout());
        GridBagConstraints c = new GridBagConstraints();
        c.insets = new Insets(4, 4, 4, 4);
        c.anchor = GridBagConstraints.WEST;
        for (int i = 0; i < labels.length; i++) {
            c.gridx = 0; c.gridy = i; c.weightx = 0; c.fill = GridBagConstraints.NONE;
            form.add(new JLabel(labels[i]), c);
            c.gridx = 1; c.weightx = 1; c.fill = GridBagConstraints.HORIZONTAL;
            form.add(fields[i], c);
        }
        int result = JOptionPane.showConfirmDialog(
            parent, form, title, JOptionPane.OK_CANCEL_OPTION, JOptionPane.PLAIN_MESSAGE);
        return result == JOptionPane.OK_OPTION;
    }
}
