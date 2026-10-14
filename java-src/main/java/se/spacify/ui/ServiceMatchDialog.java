package se.spacify.ui;

import se.spacify.service.media.ServiceMatch;
import se.spacify.ui.theme.ThemeManager;

import javax.swing.*;
import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.List;

/**
 * The "Play with…" chooser — an "Open with" dialog for a music track. Lists the
 * Skins that reported a match (icon, name and matched-track details) and lets
 * the user pick one, optionally ticking "Always play this track with this match"
 * to persist the choice. Returns the selection via {@link #choose}.
 */
public final class ServiceMatchDialog extends JDialog {

    /** The user's decision: the chosen match and whether to remember it. */
    public record Result(ServiceMatch match, boolean always) {}

    private Result result;

    private ServiceMatchDialog(Window owner, String trackLabel, List<ServiceMatch> matches) {
        super(owner, "Play with…", ModalityType.APPLICATION_MODAL);

        Color bg = ThemeManager.getBackground();
        Color fg = ThemeManager.getForeground();

        JPanel content = new JPanel(new BorderLayout(0, 10));
        content.setBorder(BorderFactory.createEmptyBorder(14, 14, 14, 14));
        content.setBackground(bg);

        JLabel heading = new JLabel("How do you want to play \"" + trackLabel + "\"?");
        heading.setFont(heading.getFont().deriveFont(Font.BOLD, 14f));
        heading.setForeground(fg);
        content.add(heading, BorderLayout.NORTH);

        JList<ServiceMatch> list = new JList<>(matches.toArray(new ServiceMatch[0]));
        list.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        list.setSelectedIndex(0);
        list.setBackground(bg);
        list.setForeground(fg);
        list.setCellRenderer(new MatchRenderer());
        list.setFixedCellHeight(44);

        JScrollPane scroll = new JScrollPane(list);
        scroll.setBorder(BorderFactory.createLineBorder(ThemeManager.getGridColor()));
        scroll.setPreferredSize(new Dimension(420, 200));
        content.add(scroll, BorderLayout.CENTER);

        JCheckBox always = new JCheckBox("Always play this track with this match");
        always.setBackground(bg);
        always.setForeground(fg);

        JButton ok     = new JButton("Play");
        JButton cancel = new JButton("Cancel");
        getRootPane().setDefaultButton(ok);

        Runnable accept = () -> {
            ServiceMatch sel = list.getSelectedValue();
            if (sel != null) {
                result = new Result(sel, always.isSelected());
                dispose();
            }
        };
        ok.addActionListener(e -> accept.run());
        cancel.addActionListener(e -> { result = null; dispose(); });
        list.addMouseListener(new MouseAdapter() {
            @Override public void mouseClicked(MouseEvent e) {
                if (e.getClickCount() == 2) accept.run();
            }
        });

        JPanel buttons = new JPanel(new FlowLayout(FlowLayout.RIGHT, 8, 0));
        buttons.setBackground(bg);
        buttons.add(cancel);
        buttons.add(ok);

        JPanel south = new JPanel(new BorderLayout());
        south.setBackground(bg);
        south.add(always, BorderLayout.WEST);
        south.add(buttons, BorderLayout.EAST);
        content.add(south, BorderLayout.SOUTH);

        setContentPane(content);
        pack();
        setLocationRelativeTo(owner);
    }

    /**
     * Show the chooser modally and return the user's decision, or {@code null} if
     * cancelled. Must be called on the EDT.
     */
    public static Result choose(Component parent, String trackLabel, List<ServiceMatch> matches) {
        Window owner = parent != null
                ? SwingUtilities.getWindowAncestor(parent)
                : KeyboardFocusManager.getCurrentKeyboardFocusManager().getActiveWindow();
        ServiceMatchDialog dialog = new ServiceMatchDialog(owner, trackLabel, matches);
        dialog.setVisible(true);
        return dialog.result;
    }

    /** Renders a match as: [Service icon] Service — matched title · artist (m:ss). */
    private static final class MatchRenderer extends DefaultListCellRenderer {
        private static final long serialVersionUID = 1L;

        @Override
        public Component getListCellRendererComponent(JList<?> list, Object value, int index,
                boolean isSelected, boolean hasFocus) {
            super.getListCellRendererComponent(list, value, index, isSelected, hasFocus);
            ServiceMatch m = (ServiceMatch) value;
            String detail = m.label() != null ? m.label() : "";
            setText("<html><b>" + escape(m.ServiceName()) + "</b>"
                    + (!detail.isEmpty() ? " — <span>" + escape(detail) + "</span>" : "")
                    + "</html>");
            setIcon(m.icon());
            setBorder(BorderFactory.createEmptyBorder(4, 8, 4, 8));
            if (isSelected) {
                setBackground(ThemeManager.getAccentForegroundColor());
                setForeground(Color.WHITE);
            } else {
                setBackground(ThemeManager.getBackground());
                setForeground(ThemeManager.getForeground());
            }
            return this;
        }

        private static String escape(String s) {
            return s == null ? "" : s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;");
        }
    }
}
