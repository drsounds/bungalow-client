package se.spacify.controls;

import se.spacify.service.media.PlayQueue;
import se.spacify.ui.theme.ThemeManager;

import javax.swing.BorderFactory;
import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.ImageIcon;
import javax.swing.JComponent;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.SwingConstants;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.Image;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.ArrayList;
import java.util.List;

/**
 * A vertically-stacked, grouped presentation of playable rows: each group is a
 * Spotify-artist-profile-style section with the context image on the left and
 * the title and subtitle stretched to its right, followed by the group's item
 * rows. A row is played by double-clicking it; the currently-playing row is
 * highlighted and tracks the shared {@link PlayQueue}.
 *
 * <p>Used as the alternative viewport content for the library views when the
 * user switches a view into its grouped mode.
 */
public class GroupedListPanel extends Panel {

    private static final int IMAGE_SIZE = 64;

    /** One playable line within a group. */
    public static final class Item {
        final String title;
        final String subtitle;
        final String trailing;
        final String nowPlayingKey;
        final Runnable onActivate;

        public Item(String title, String subtitle, String trailing,
                    String nowPlayingKey, Runnable onActivate) {
            this.title         = title    != null ? title    : "";
            this.subtitle      = subtitle != null ? subtitle : "";
            this.trailing      = trailing != null ? trailing : "";
            this.nowPlayingKey = nowPlayingKey;
            this.onActivate    = onActivate;
        }
    }

    /** A titled section with a context image and its item rows. */
    public static final class Group {
        final Image image;
        final String title;
        final String subtitle;
        final List<Item> items;

        public Group(Image image, String title, String subtitle, List<Item> items) {
            this.image    = image;
            this.title    = title    != null ? title    : "";
            this.subtitle = subtitle != null ? subtitle : "";
            this.items    = items;
        }
    }

    private final List<ItemRow> rowComponents = new ArrayList<>();

    public GroupedListPanel() {
        getComponent().setLayout(new BoxLayout(getComponent(), BoxLayout.Y_AXIS));
        getComponent().setOpaque(true);
        ThemeManager.addChangeListener(this::applyColors);
        // Follow the active track so the now-playing highlight moves with playback.
        PlayQueue.getInstance().addChangeListener(this::repaintRows);
    }

    /** Replace the rendered content with {@code groups}. */
    public void setGroups(List<Group> groups) {
        getComponent().removeAll();
        rowComponents.clear();
        for (Group group : groups) {
            getComponent().add(buildHeader(group));
            // Stripe rows within the section, restarting each group so every
            // section opens on the same base shade under its header.
            int index = 0;
            for (Item item : group.items) {
                ItemRow row = new ItemRow(item, index++);
                rowComponents.add(row);
                getComponent().add(row);
            }
            getComponent().add(Box.createVerticalStrut(18));
        }
        applyColors();
        revalidate();
        repaint();
    }

    private JComponent buildHeader(Group group) {
        JPanel header = new JPanel(new BorderLayout(12, 0));
        header.setOpaque(false);
        header.setBorder(BorderFactory.createEmptyBorder(10, 6, 8, 6));
        header.setAlignmentX(Component.LEFT_ALIGNMENT);

        JLabel image = new JLabel();
        if (group.image != null) {
            image.setIcon(new ImageIcon(group.image.getScaledInstance(
                    IMAGE_SIZE, IMAGE_SIZE, Image.SCALE_SMOOTH)));
        }
        image.setPreferredSize(new Dimension(IMAGE_SIZE, IMAGE_SIZE));
        header.add(image, BorderLayout.WEST);

        // Title + subtitle stack, stretched across the remaining width.
        JPanel text = new JPanel();
        text.setOpaque(false);
        text.setLayout(new BoxLayout(text, BoxLayout.Y_AXIS));
        text.setBorder(BorderFactory.createEmptyBorder(4, 0, 0, 0));

        JLabel title = new JLabel(group.title);
        title.setFont(title.getFont().deriveFont(Font.BOLD, 17f));
        title.setForeground(ThemeManager.getForeground());
        title.setAlignmentX(Component.LEFT_ALIGNMENT);

        JLabel subtitle = new JLabel(group.subtitle);
        subtitle.setFont(subtitle.getFont().deriveFont(Font.PLAIN, 12f));
        subtitle.setForeground(muted());
        subtitle.setAlignmentX(Component.LEFT_ALIGNMENT);

        text.add(Box.createVerticalGlue());
        text.add(title);
        if (!group.subtitle.isEmpty()) text.add(subtitle);
        text.add(Box.createVerticalGlue());
        header.add(text, BorderLayout.CENTER);

        // Cap the header height so the section reads as a band, not a tall block.
        header.setMaximumSize(new Dimension(Integer.MAX_VALUE, IMAGE_SIZE + 18));
        return header;
    }

    private void repaintRows() {
        for (ItemRow row : rowComponents) row.repaint();
    }

    private void applyColors() {
        getComponent().setBackground(ThemeManager.getBackground());
        for (Component c : getComponent().getComponents()) {
            if (c instanceof JComponent jc) jc.setForeground(ThemeManager.getForeground());
        }
        for (ItemRow row : rowComponents) row.refreshColors();
        repaint();
    }

    private static Color muted() {
        Color fg = ThemeManager.getForeground();
        return new Color(fg.getRed(), fg.getGreen(), fg.getBlue(), 160);
    }

    /** A single playable line: title + subtitle on the left, trailing on the right. */
    private final class ItemRow extends JPanel {
        private static final long serialVersionUID = 1L;

        private final Item item;
        private final int index;
        private final JLabel primary;
        private final JLabel secondary;
        private final JLabel trailing;
        private boolean hover;

        ItemRow(Item item, int index) {
            super(new BorderLayout(10, 0));
            this.item = item;
            this.index = index;
            setOpaque(true);
            setBorder(BorderFactory.createEmptyBorder(4, IMAGE_SIZE + 18, 4, 12));
            setAlignmentX(LEFT_ALIGNMENT);
            setMaximumSize(new Dimension(Integer.MAX_VALUE, 38));

            JPanel left = new JPanel();
            left.setOpaque(false);
            left.setLayout(new BoxLayout(left, BoxLayout.Y_AXIS));
            primary = new JLabel(item.title);
            primary.setAlignmentX(LEFT_ALIGNMENT);
            secondary = new JLabel(item.subtitle);
            secondary.setFont(secondary.getFont().deriveFont(Font.PLAIN, 11f));
            secondary.setAlignmentX(LEFT_ALIGNMENT);
            left.add(primary);
            if (!item.subtitle.isEmpty()) left.add(secondary);
            add(left, BorderLayout.CENTER);

            trailing = new JLabel(item.trailing, SwingConstants.RIGHT);
            trailing.setFont(trailing.getFont().deriveFont(Font.PLAIN, 12f));
            add(trailing, BorderLayout.EAST);

            addMouseListener(new MouseAdapter() {
                @Override public void mouseEntered(MouseEvent e) { hover = true;  repaint(); }
                @Override public void mouseExited(MouseEvent e)  { hover = false; repaint(); }
                @Override public void mouseClicked(MouseEvent e) {
                    if (e.getClickCount() == 2 && item.onActivate != null) item.onActivate.run();
                }
            });
            refreshColors();
        }

        void refreshColors() {
            primary.setForeground(ThemeManager.getForeground());
            secondary.setForeground(muted());
            trailing.setForeground(muted());
        }

        private boolean isNowPlaying() {
            return item.nowPlayingKey != null
                    && PlayQueue.getInstance().isCurrentKey(item.nowPlayingKey);
        }

        @Override
        public Color getBackground() {
            // getBackground() is queried by the superclass paint before our fields
            // exist during construction; guard against that.
            if (item == null) return ThemeManager.getBackground();
            if (isNowPlaying()) return ThemeManager.getNowPlayingBackground();
            // Zebra striping, matching the flat table's alternating rows.
            Color base = (index % 2 == 0)
                    ? ThemeManager.getBackground()
                    : ThemeManager.getAlternateBackground();
            return hover ? hover(base) : base;
        }

        /** A subtle accent-tinted shade for the hovered row, distinct from stripes. */
        private static Color hover(Color base) {
            Color a = ThemeManager.getAccentForegroundColor();
            return new Color((base.getRed()   * 3 + a.getRed())   / 4,
                             (base.getGreen() * 3 + a.getGreen()) / 4,
                             (base.getBlue()  * 3 + a.getBlue())  / 4);
        }
    }
}
