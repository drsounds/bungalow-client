package se.spacify.app.media.views;

import se.spacify.navigation.PlayerView;

import javax.swing.*;
import java.awt.*;

public class DefaultPlayerView extends PlayerView {

    private final JPanel panel;
    private final JLabel trackLabel;
    private final JLabel artistLabel;
    private final JLabel albumLabel;

    public DefaultPlayerView() {
        panel = new JPanel(new BorderLayout());
        panel.setOpaque(false);

        JPanel albumArt = new JPanel() {
            @Override
            protected void paintComponent(Graphics g) {
                super.paintComponent(g);
                Graphics2D g2 = (Graphics2D) g.create();
                g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
                int w = getWidth(), h = getHeight();
                int size = Math.min(w, h) - 32;
                int x = (w - size) / 2, y = (h - size) / 2;
                GradientPaint grad = new GradientPaint(x, y, new Color(50, 50, 62), x + size, y + size, new Color(22, 22, 30));
                g2.setPaint(grad);
                g2.fillRoundRect(x, y, size, size, 20, 20);
                g2.setColor(new Color(255, 255, 255, 55));
                g2.setFont(new Font("Dialog", Font.PLAIN, size / 3));
                FontMetrics fm = g2.getFontMetrics();
                String note = "♪";
                g2.drawString(note, x + (size - fm.stringWidth(note)) / 2,
                              y + (size + fm.getAscent() - fm.getDescent()) / 2);
                g2.dispose();
            }
        };
        albumArt.setOpaque(false);
        albumArt.setPreferredSize(new Dimension(240, 240));

        trackLabel = new JLabel("No track playing", SwingConstants.CENTER);
        trackLabel.setFont(trackLabel.getFont().deriveFont(Font.BOLD, 18f));
        trackLabel.setForeground(Color.WHITE);

        artistLabel = new JLabel("", SwingConstants.CENTER);
        artistLabel.setFont(artistLabel.getFont().deriveFont(14f));
        artistLabel.setForeground(new Color(180, 180, 180));

        albumLabel = new JLabel("", SwingConstants.CENTER);
        albumLabel.setFont(albumLabel.getFont().deriveFont(Font.ITALIC, 12f));
        albumLabel.setForeground(new Color(130, 130, 130));

        JPanel info = new JPanel();
        info.setLayout(new BoxLayout(info, BoxLayout.Y_AXIS));
        info.setOpaque(false);
        info.setBorder(BorderFactory.createEmptyBorder(14, 0, 0, 0));
        for (JLabel lbl : new JLabel[]{trackLabel, artistLabel, albumLabel}) {
            lbl.setAlignmentX(Component.CENTER_ALIGNMENT);
            info.add(lbl);
            info.add(Box.createVerticalStrut(3));
        }

        JPanel center = new JPanel();
        center.setLayout(new BoxLayout(center, BoxLayout.Y_AXIS));
        center.setOpaque(false);
        albumArt.setAlignmentX(Component.CENTER_ALIGNMENT);
        info.setAlignmentX(Component.CENTER_ALIGNMENT);
        center.add(Box.createVerticalGlue());
        center.add(albumArt);
        center.add(info);
        center.add(Box.createVerticalGlue());

        panel.add(center, BorderLayout.CENTER);
    }

    @Override public String getName() { return "Default"; }
    @Override public JComponent getComponent() { return panel; }

    @Override
    public void onTrackChanged(String title, String artist, String album) {
        trackLabel.setText(title  != null && !title.isEmpty()  ? title  : "No track playing");
        artistLabel.setText(artist != null ? artist : "");
        albumLabel.setText(album  != null ? album  : "");
    }
}
