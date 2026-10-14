package se.spacify.ui.theme;

import javax.swing.*;

import se.spacify.aspect.BaseAspectManager;
import se.spacify.service.AuthAspect;
import se.spacify.ui.MainWindow;

import java.awt.*;
import java.util.ArrayList;
import java.util.List;

public class ThemeManager extends BaseAspectManager<Theme> {

    public ThemeManager(MainWindow mainWindow) {
        super(mainWindow);
        //TODO Auto-generated constructor stub
    }

    // ── Theme registration ──────────────────────────────────────────────────

    /**
     * Returns the first registered theme exposing the given aspect, or null.
     * {@code aspect} may be {@link Theme} itself, a concrete theme class, or
     * any capability interface (e.g. {@link se.spacify.theme.media.MediaTheme},
     * {@link AuthAspect}); only the themes implementing it are considered.
     */
    @SuppressWarnings("unchecked")
    public <T> T getTheme(Class<T> aspect) {
        for (Theme s : getNodes().values())
            if (aspect.isInstance(s)) return (T) s;
        return null;
    }

    /** Returns all registered themes exposing the given aspect. */
    @SuppressWarnings("unchecked")
    public <T> List<T> getThemes(Class<T> aspect) {
        List<T> result = new ArrayList<>();
        for (Theme s : getNodes().values())
            if (aspect.isInstance(s)) result.add((T) s);
        return result;
    }

    private static float hue        = 0.0f;  // 0-1  (background tint)
    private static float saturation = 0.0f;  // 0-1  (background tint)
    private static float lightness  = 0.5f;  // 0-1  (background tint)
    private static boolean darkMode = true;
    private static Color accentForegroundColor = new Color(30, 215, 96);  // Spotify green default

    // ── Display toggles (saved alongside the HSL / dark-light / accent settings) ──
    private static boolean stripedRows          = true;   // alternate row shading
    private static boolean fillEmptyRows        = true;   // continue the zebra below the last row
    private static boolean highContrast         = false;  // plain B/W background (WMP-style)
    private static boolean highContrastInverted = false;  // white-on-black vs black-on-white
    private static boolean tintText             = true;   // tint text in light mode (else black)

    private static final List<Runnable> listeners = new ArrayList<>();

    // Cached per applyToDefaults() — safe to read from any component at render time
    private static Color currentBg    = new Color(22, 22, 22);
    private static Color currentAltBg = new Color(30, 30, 30);
    private static Color currentFg    = new Color(210, 210, 210);
    private static Color currentGrid  = new Color(35, 35, 35);
    private static Color accentBackgroundColor;

    public static Color getBackground()          { return currentBg; }
    public static Color getAlternateBackground() { return currentAltBg; }
    public static Color getForeground()           { return currentFg; }
    public static Color getGridColor()            { return currentGrid; }

    // Fixed highlight for the row matching the currently-playing track,
    // applied consistently across the play-queue and all library/list views.
    private static final Color NOW_PLAYING_BG = Color.BLACK;
    private static final Color NOW_PLAYING_FG = new Color(144, 238, 144);  // light green

    public static Color getNowPlayingBackground() { return NOW_PLAYING_BG; }
    public static Color getNowPlayingForeground() { return NOW_PLAYING_FG; }

    public static void setHue(float h)           { hue = h;           applyToDefaults(); notify_(); }
    public static void setSaturation(float s)    { saturation = s;    applyToDefaults(); notify_(); }
    public static void setLightness(float l)     { lightness = l;     applyToDefaults(); notify_(); }
    public static void setDarkMode(boolean d)    { darkMode = d;      applyToDefaults(); notify_(); }
    public static void setAccentForegroundColor(Color c)   { accentForegroundColor = c;   applyToDefaults(); notify_(); }
    public static void setAccentBackgroundColor(Color c)   { accentBackgroundColor = c;   applyToDefaults(); notify_(); }

    public static void setStripedRows(boolean v)          { stripedRows = v;          applyToDefaults(); notify_(); }
    public static void setFillEmptyRows(boolean v)        { fillEmptyRows = v;        notify_(); }
    public static void setHighContrast(boolean v)         { highContrast = v;         applyToDefaults(); notify_(); }
    public static void setHighContrastInverted(boolean v) { highContrastInverted = v; applyToDefaults(); notify_(); }
    public static void setTintText(boolean v)             { tintText = v;             applyToDefaults(); notify_(); }

    public static float   getHue()          { return hue; }
    public static float   getSaturation()   { return saturation; }
    public static float   getLightness()    { return lightness; }
    public static boolean isDarkMode()      { return darkMode; }
    public static Color   getAccentForegroundColor()  { return accentForegroundColor; }
    public static Color   getAccentBackgroundColor()  { return accentBackgroundColor; }

    public static boolean isStripedRows()          { return stripedRows; }
    public static boolean isFillEmptyRows()        { return fillEmptyRows; }
    public static boolean isHighContrast()         { return highContrast; }
    public static boolean isHighContrastInverted() { return highContrastInverted; }
    public static boolean isTintText()             { return tintText; }

    public static void addChangeListener(Runnable r) { listeners.add(r); }

    /** Call once at startup and on every theme change to seed UIManager. */
    public static void applyToDefaults() {
        UIDefaults d = UIManager.getLookAndFeelDefaults();
        if (highContrast) {
            // Plain black-and-white scheme, à la the classic WMP display option.
            currentBg    = highContrastInverted ? Color.BLACK : Color.WHITE;
            currentFg    = highContrastInverted ? Color.WHITE : Color.BLACK;
            currentAltBg = highContrastInverted ? new Color(28, 28, 28) : new Color(228, 228, 228);
            currentGrid  = highContrastInverted ? new Color(60, 60, 60) : new Color(200, 200, 200);
        } else if (darkMode) {
            float bg  = 0.07f + lightness * 0.50f;

            currentBg    = hsl(hue, saturation * 0.75f, bg);
            currentAltBg = hsl(hue, saturation * 0.75f, Math.min(1f, bg + 0.04f));
            currentFg    = new Color(210, 210, 210);
            currentGrid  = hsl(hue, saturation * 0.75f, Math.min(1f, bg + 0.06f));
        } else {
            float bg  = 0.84f + lightness * 0.28f;

            currentBg    = hsl(hue, saturation * 0.48f, bg);
            currentAltBg = hsl(hue, saturation * 0.08f, Math.max(0f, bg - 0.04f));
            // Light mode: tint the text with the chosen hue, or fall back to black.
            currentFg    = tintText ? hsl(hue, saturation, 0.45f) : Color.BLACK;
            currentGrid  = hsl(hue, saturation * 0.08f, Math.max(0f, bg - 0.06f));
        }

        // Striped-rows toggle: collapse the alternate row colour onto the base when off.
        if (!stripedRows) currentAltBg = currentBg;

        // Shared keys
        float acc = darkMode ? (0.28f + lightness * 0.10f) : (0.42f + lightness * 0.10f);
        float mid = darkMode ? (0.07f + lightness * 0.50f + 0.05f) : 0.68f;

        put(d, "control",        currentBg);
        put(d, "nimbusBase",     hsl(hue, saturation, acc));
        put(d, "nimbusBlueGrey", hsl(hue, saturation, mid));
        put(d, "text",           currentFg);
        put(d, "textForeground", currentFg);
        put(d, "nimbusDisabledText",          darkMode ? new Color(100,100,100) : new Color(120,120,120));
        put(d, "nimbusSelectedText",          darkMode ? Color.WHITE : Color.BLACK);
        put(d, "nimbusFocus",               accentForegroundColor);
        put(d, "nimbusSelectionBackground", accentForegroundColor);

        // Component-specific keys read by DefaultTreeCellRenderer/JTable.updateUI()
        // and by our themed renderers as fallback
        put(d, "Tree.textBackground",       currentBg);
        put(d, "Tree.textForeground",        currentFg);
        put(d, "Tree.selectionBackground",   accentForegroundColor);
        put(d, "Tree.selectionForeground",   Color.WHITE);
        put(d, "Tree.selectionBorderColor",  accentForegroundColor);
        put(d, "Table.background",           currentBg);
        put(d, "Table.foreground",           currentFg);
        put(d, "Table.selectionBackground",  accentForegroundColor);
        put(d, "Table.selectionForeground",  Color.WHITE);
        put(d, "Table.gridColor",            currentGrid);
        put(d, "Table.rowHeight",            24);   // Windows ListView-style row height
        put(d, "TableHeader.background",     new Color(235, 234, 219));
        put(d, "TableHeader.foreground",     new Color(0, 0, 0));
        put(d, "List.background",            currentBg);
        put(d, "List.foreground",            currentFg);
        put(d, "List.selectionBackground",   accentForegroundColor);
        put(d, "List.selectionForeground",   Color.WHITE);
        put(d, "ScrollPane.background",      currentBg);
        put(d, "Viewport.background",        currentBg);
        put(d, "EditorPane.background",      currentBg);
        put(d, "EditorPane.foreground",      currentFg);
        put(d, "TextArea.background",        currentBg);
        put(d, "TextArea.foreground",        currentFg);

        applyUiFont(d);
    }

    /** The app-wide UI font: Tahoma 11 where available, else a sans-serif fallback. */
    private static Font uiFont;
    public static Font getUiFont() {
        if (uiFont == null) {
            boolean hasTahoma = false;
            for (String fam : GraphicsEnvironment.getLocalGraphicsEnvironment().getAvailableFontFamilyNames()) {
                if (fam.equalsIgnoreCase("Tahoma")) { hasTahoma = true; break; }
            }
            uiFont = new Font(hasTahoma ? "Tahoma" : Font.SANS_SERIF, Font.PLAIN, 11);
        }
        return uiFont;
    }

    /**
     * Force every component's font to {@link #getUiFont()} (11px Tahoma). Sets
     * Nimbus's {@code defaultFont} and every {@code *.font} default — so trees,
     * tables, their headers, lists and everything else share one font. Re-run on
     * each {@link #applyToDefaults()} because reinstalling Nimbus resets fonts.
     */
    private static void applyUiFont(UIDefaults d) {
        javax.swing.plaf.FontUIResource f = new javax.swing.plaf.FontUIResource(getUiFont());
        //put(d, "defaultFont", f);                 // Nimbus applies this everywhere
        for (Object key : new ArrayList<>(d.keySet())) {
            if (key instanceof String s && s.endsWith("Table.font")) {
                put(d, s, f);                     // explicit per-component fonts (Table.font, Tree.font, …)
            }
            if (key instanceof String s && s.endsWith("Tree.font")) {
                put(d, s, f);                     // explicit per-component fonts (Table.font, Tree.font, …)
            }
        }
    }

    /** Scale a colour's RGB channels by {@code factor} (1.0 = unchanged), preserving alpha. */
    public static Color scaleRgb(Color base, float factor) {
        if (factor >= 1f) return base;
        return new Color(
            Math.round(base.getRed()   * factor),
            Math.round(base.getGreen() * factor),
            Math.round(base.getBlue()  * factor),
            base.getAlpha());
    }
    
    /** Color derived from the background-tint HSL sliders — used for chrome gradients. */
    @SuppressWarnings("unused")
	public static Color getTintColor() {
        float l = false ? (0.28f + lightness * 0.10f) : (0.42f + lightness * 0.10f);
        return hsl(hue, saturation, l);
    }

    /** Darken the tint: each channel multiplied by ratio (0–1). */
    public static Color accentDark(float ratio) {
        return ColorUtils.darken(getAccentForegroundColor(), ratio);
    }

    /** Darken the tint: each channel multiplied by ratio (0–1). */
    public static Color tintDark(float ratio) {
        return ColorUtils.darken(getTintColor(), ratio);
    }

    /** Brighten the tint: each channel multiplied by ratio (>1 boosts toward white). */
    public static Color accentLight(float ratio) {
        return ColorUtils.lighten(getAccentForegroundColor(), ratio);
    }

    /** Brighten the tint: each channel multiplied by ratio (>1 boosts toward white). */
    public static Color tintLight(float ratio) {
        return ColorUtils.lighten(getTintColor(), ratio);
    }

    // ── HSL conversion ────────────────────────────────────────────────────────

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

    private static void put(UIDefaults d, String key, Object val) {
        d.put(key, val);
        UIManager.put(key, val);   // also override in the global layer
    }

    private static void notify_() {
        for (Runnable r : listeners) r.run();
    }
}
