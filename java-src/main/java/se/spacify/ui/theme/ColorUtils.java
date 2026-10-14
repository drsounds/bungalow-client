package se.spacify.ui.theme;

import java.awt.Color;

public class ColorUtils {
	public static Color alpha(Color c, float opacity) {
		return new Color(c.getRed() / 255, c.getGreen() / 255, c.getBlue() / 255, opacity / 255);
	}
	public static Color lighten(Color a, float ratio) {
        return new Color(
            Math.min(255, (int)(a.getRed()   * ratio)),
            Math.min(255, (int)(a.getGreen() * ratio)),
            Math.min(255, (int)(a.getBlue()  * ratio))
        );
	}
	public static Color darken(Color a, float ratio) {
        return new Color(
            Math.min(255, (int)(a.getRed()   * ratio)),
            Math.min(255, (int)(a.getGreen() * ratio)),
            Math.min(255, (int)(a.getBlue()  * ratio))
        );
	}
	/**
     * Modifies the saturation of an existing Swing Color.
     * * @param baseColor The original java.awt.Color.
     * @param saturationFactor The multiplier for saturation (e.g., 0.5f to fade, 1.5f to boost).
     * @return A new java.awt.Color object with adjusted saturation.
     */
    public static Color saturate(Color baseColor, float saturationFactor) {
        // 1. Convert RGB to HSB
        float[] hsb = Color.RGBtoHSB(baseColor.getRed(), baseColor.getGreen(), baseColor.getBlue(), null);
        
        float hue = hsb[0];
        float saturation = hsb[1];
        float brightness = hsb[2];

        // 2. Adjust saturation and clamp between 0.0 and 1.0
        saturation = saturation * saturationFactor;
        saturation = Math.max(0.0f, Math.min(1.0f, saturation));

        // 3. Return the new Color using the HSB factory method (keeps alpha/transparency if needed)
        int rgb = Color.HSBtoRGB(hue, saturation, brightness);
        
        // If you want to preserve the original color's alpha (transparency):
        int alpha = baseColor.getAlpha();
        return new Color((rgb & 0x00FFFFFF) | (alpha << 24), true);
    }
    
    public static double getLuminanceOfColor(Color color) {
        return 0.2126 * color.getRed() + 0.7152 * color.getGreen() + 0.0722 * color.getBlue();
    }

    public static Color hsl(float h, float s, float l) {
        if (s == 0f) { int v = (int)(l * 255); return new Color(v, v, v); }
        float q = l < 0.5f ? l * (1 + s) : l + s - l * s;
        float p = 2 * l - q;
        return new Color(
            clamp(hueToRgb(p, q, h + 1f / 3)),
            clamp(hueToRgb(p, q, h)),
            clamp(hueToRgb(p, q, h - 1f / 3))
        );
    }

    private static float hueToRgb(float p, float q, float t) {
        if (t < 0) t += 1; if (t > 1) t -= 1;
        if (t < 1f/6) return p + (q - p) * 6 * t;
        if (t < 1f/2) return q;
        if (t < 2f/3) return p + (q - p) * (2f/3 - t) * 6;
        return p;
    }

    private static float clamp(float v) { return Math.max(0, Math.min(1, v)); }

}
