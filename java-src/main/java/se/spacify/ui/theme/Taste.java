package se.spacify.ui.theme;

import java.awt.Color;
import java.util.ArrayList;

import se.spacify.design.Design;
import se.spacify.skinning.Skin;
import se.spacify.ui.chrome.Chrome;

public class Taste {
    private ArrayList<Runnable> listeners;
    private Theme theme;
    private Design design;
    public Design getDesign() {
        if (theme != null) {
            if (theme.getDesign() != null) return theme.getDesign();
        }
        return design;
    }
    @SuppressWarnings("unused")
    public Color getTintColor() {
        float l = false ? (0.28f + lightness * 0.10f) : (0.42f + lightness * 0.10f);
        return ColorUtils.hsl(hue, saturation, l);
    }
    public void setDesign(Design design) {
        this.design = design;
        // Notify so the config::save listener persists the chosen Design.
        notify_();
    }
    private Skin skin;
    public Skin getSkin() {
        if (theme != null) {
            if (theme.getSkin() != null) return theme.getSkin();
        }
        return skin;
    }
    public void setSkin(Skin skin) {
        this.skin = skin;
    }
    private Chrome chrome;
    public Chrome getChrome() {
        if (theme != null) {
            if (theme.getChrome() != null) return theme.getChrome();
        }
        return chrome;
    }
    public void setChrome(Chrome chrome) {
        this.chrome = chrome;
    }
    public Theme getTheme() {
        return theme;
    }
    public void setTheme(Theme theme) {
        this.theme = theme;
    }
    public Taste() {
        listeners = new ArrayList<>();
    }
    private float hue        = 0.0f;  // 0-1  (background tint)
    private float saturation = 0.0f;  // 0-1  (background tint)
    private float lightness  = 0.5f;  // 0-1  (background tint)
    private boolean darkMode = true;
    private Color accentBackgroundColor = new Color(30, 215, 96);  // Spotify green default
    private Color accentForegroundColor = new Color(0, 0, 0, 0);

    public float getAlternateBackgroundShade() {
        return currentAltBgShade;
    }
    public void setAlternateBackgroundShade(float value) {
        this.currentAltBgShade = value;
        this.setAlternateBackground(ColorUtils.lighten(currentAltBg, value));
    }
    // ── Display toggles (saved alongside the HSL / dark-light / accent settings) ──
    private boolean stripedRows          = true;   // alternate row shading
    private boolean fillEmptyRows        = true;   // continue the zebra below the last row
    private boolean highContrast         = false;  // plain B/W background (WMP-style)
    private boolean highContrastInverted = false;  // white-on-black vs black-on-white
    private boolean tintText             = true;   // tint text in light mode (else black)

    // Cached per applyToDefaults() — safe to read from any component at render time
    private Color currentBg    = new Color(22, 22, 22);
    private Color currentAltBg = new Color(30, 30, 30);
    private Color currentFg    = new Color(210, 210, 210);
    private Color currentGrid  = new Color(35, 35, 35);
    private float currentAltBgShade = 1.1f;

    public Color getBackground()          { return currentBg; }
    public Color getAlternateBackground() { return currentAltBg; }
    public void setAlternateBackground(Color value) {
        currentAltBg = value;
    }
    public Color getForeground()           { return currentFg; }
    public Color getGridColor()            { return currentGrid; }

    // Fixed highlight for the row matching the currently-playing track,
    // applied consistently across the play-queue and all library/list views.
    private final Color NOW_PLAYING_BG = Color.BLACK;
    private final Color NOW_PLAYING_FG = new Color(144, 238, 144);  // light green

    public Color getNowPlayingBackground() { return NOW_PLAYING_BG; }
    public Color getNowPlayingForeground() { return NOW_PLAYING_FG; }

    // The Taste is the model the UI (sliders, config) writes to, but the whole
    // app renders from ThemeManager's cached colours and its change listeners.
    // Forward every change into ThemeManager (which recomputes the palette and
    // repaints all listeners) and then fire our own listeners (theme rebuild,
    // config save) — otherwise a tint change would reach only the few elements
    // that read the Taste directly.
    public void setHue(float h)           { hue = h;           ThemeManager.setHue(h);           notify_(); }
    public void setSaturation(float s)    { saturation = s;    ThemeManager.setSaturation(s);    notify_(); }
    public void setLightness(float l)     { lightness = l;     ThemeManager.setLightness(l);     notify_(); }
    public void setDarkMode(boolean d)    { darkMode = d;      ThemeManager.setDarkMode(d);      notify_(); }
    public void setAccentBackgroundColor(Color c)   { accentBackgroundColor = c;   ThemeManager.setAccentBackgroundColor(c);   notify_(); }
    public void setAccentForegroundColor(Color c)   { accentForegroundColor = c;   ThemeManager.setAccentForegroundColor(c);   notify_(); }

    public void setStripedRows(boolean v)          { stripedRows = v;          ThemeManager.setStripedRows(v);          notify_(); }
    public void setFillEmptyRows(boolean v)        { fillEmptyRows = v;        ThemeManager.setFillEmptyRows(v);        notify_(); }
    public void setHighContrast(boolean v)         { highContrast = v;         ThemeManager.setHighContrast(v);         notify_(); }
    public void setHighContrastInverted(boolean v) { highContrastInverted = v; ThemeManager.setHighContrastInverted(v); notify_(); }
    public void setTintText(boolean v)             { tintText = v;             ThemeManager.setTintText(v);             notify_(); }

    public float   getHue()          { return hue; }
    public float   getSaturation()   { return saturation; }
    public float   getLightness()    { return lightness; }
    public boolean isDarkMode()      { return darkMode; }
    public Color   getAccentBackgroundColor()  { return accentBackgroundColor; }
    public Color   getAccentForegroundColor() { return accentForegroundColor; }

    public boolean isStripedRows()          { return stripedRows; }
    public boolean isFillEmptyRows()        { return fillEmptyRows; }
    public boolean isHighContrast()         { return highContrast; }
    public boolean isHighContrastInverted() { return highContrastInverted; }
    public boolean isTintText()             { return tintText; }

    public void addChangeListener(Runnable r) { listeners.add(r); }
    public void notify_() {
        for (Runnable r : listeners) r.run();
    }
}
