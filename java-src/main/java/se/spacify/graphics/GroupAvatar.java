package se.spacify.graphics;

import java.awt.Color;
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.GradientPaint;
import java.awt.Graphics2D;
import java.awt.Image;
import java.awt.RenderingHints;
import java.awt.image.BufferedImage;

/**
 * Generates a deterministic square placeholder image for a grouping context
 * (a release, a playlist, …) when no real cover art is available. The colour is
 * derived from the group's stable key so the same group always renders the same
 * tile, and the first letters of its label are drawn on top — the kind of
 * coloured initial tile shown next to sections on a Spotify artist profile.
 */
public final class GroupAvatar {

    private GroupAvatar() {}

    /**
     * @param key   stable identity of the group; drives the hue so a group keeps
     *              its colour across reloads
     * @param label human-readable name; its leading initials are drawn on the tile
     * @param size  width and height of the returned square image, in pixels
     */
    public static Image of(String key, String label, int size) {
        BufferedImage img = new BufferedImage(size, size, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g = img.createGraphics();
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
        g.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, RenderingHints.VALUE_TEXT_ANTIALIAS_ON);

        // Diagonal gradient between two shades of a hue picked from the key.
        float hue = ((key == null ? 0 : key.hashCode()) & 0x7fffffff) % 360 / 360f;
        Color top    = Color.getHSBColor(hue, 0.55f, 0.78f);
        Color bottom = Color.getHSBColor(hue, 0.65f, 0.50f);
        g.setPaint(new GradientPaint(0, 0, top, size, size, bottom));
        g.fillRect(0, 0, size, size);

        String initials = initials(label);
        if (!initials.isEmpty()) {
            g.setColor(new Color(255, 255, 255, 230));
            g.setFont(new Font("SansSerif", Font.BOLD, Math.max(10, size / 2)));
            FontMetrics fm = g.getFontMetrics();
            int x = (size - fm.stringWidth(initials)) / 2;
            int y = (size - fm.getHeight()) / 2 + fm.getAscent();
            g.drawString(initials, x, y);
        }
        g.dispose();
        return img;
    }

    /** Up to two uppercase initials from the first words of {@code label}. */
    private static String initials(String label) {
        if (label == null) return "";
        StringBuilder sb = new StringBuilder();
        for (String word : label.trim().split("\\s+")) {
            if (word.isEmpty()) continue;
            sb.append(Character.toUpperCase(word.charAt(0)));
            if (sb.length() == 2) break;
        }
        return sb.toString();
    }
}
