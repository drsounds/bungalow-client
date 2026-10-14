package se.spacify.config;

import se.spacify.ui.MainWindow;
import se.spacify.ui.theme.ColorUtils;
import se.spacify.ui.theme.Taste;
import se.spacify.ui.theme.Theme;

import java.awt.Color;
import java.io.*;
import java.nio.file.*;
import java.util.Properties;

public class ConfigManager {

    MainWindow mainWindow;
    public Theme getTheme() {
        return mainWindow.getTheme();
    }
    public Taste getTaste() {
        return mainWindow.getTaste();
    }
    public void setTaste(Taste taste) {
        mainWindow.setTaste(taste);
    }
    public ConfigManager(MainWindow mainWindow) {
        this.mainWindow = mainWindow;
    }

    private static final Path CONFIG_FILE =
        Path.of(System.getProperty("user.home"), ".spacify", "settings.properties");

    public void load() {
        Taste taste = new Taste();
        if (!Files.exists(CONFIG_FILE)) return;
            Properties p = new Properties();
        try (InputStream in = Files.newInputStream(CONFIG_FILE)) {
            p.load(in);
        } catch (IOException e) {
            return;
        }
        try {
            String themeId = p.getProperty("theme.id", "wmp10");
             Theme theme = mainWindow.getThemeManager().get(themeId);
             if (theme != null) {
                 taste.setTheme(theme);
             }
            // Restore the saved Design (Chrome + Skin premix); MainWindow installs
            // it after plugins register, via pickInitialDesign().
            String designId = p.getProperty("design.id", "");
            if (!designId.isBlank()) {
                se.spacify.design.Design design = mainWindow.getDesignManager().get(designId);
                if (design != null) taste.setDesign(design);
            }
            taste.setHue(        Float.parseFloat(p.getProperty("taste.hue",        "0.0")));
            taste.setSaturation( Float.parseFloat(p.getProperty("taste.saturation", "0.0")));
            taste.setLightness(  Float.parseFloat(p.getProperty("taste.lightness",  "0.5")));
            taste.setHue(        Float.parseFloat(p.getProperty("theme.hue",        "0.0")));
            taste.setSaturation( Float.parseFloat(p.getProperty("theme.saturation", "0.0")));
            taste.setLightness(  Float.parseFloat(p.getProperty("theme.lightness",  "0.5")));
            taste.setDarkMode(  Boolean.parseBoolean(p.getProperty("theme.darkMode", "true")));

            Color accentBackgroundColor = new Color(
                Integer.parseInt(p.getProperty("theme.accentColor",
                    String.valueOf(new Color(30, 215, 96).getRGB()))), true);
            taste.setAccentBackgroundColor(accentBackgroundColor);

            double luminance = ColorUtils.getLuminanceOfColor(accentBackgroundColor);
        
            Color accentForegroundColor = luminance > 127 ? accentBackgroundColor.darker().darker() : accentBackgroundColor.brighter();
            taste.setAccentForegroundColor(accentForegroundColor);
        
            taste.setStripedRows(         Boolean.parseBoolean(p.getProperty("theme.stripedRows",          "true")));
            taste.setFillEmptyRows(       Boolean.parseBoolean(p.getProperty("theme.fillEmptyRows",        "true")));
            taste.setHighContrast(        Boolean.parseBoolean(p.getProperty("theme.highContrast",         "false")));
            taste.setHighContrastInverted(Boolean.parseBoolean(p.getProperty("theme.highContrastInverted", "false")));
            taste.setTintText(            Boolean.parseBoolean(p.getProperty("theme.tintText",            "true")));
        } catch (NumberFormatException ignored) {
            // corrupted config — keep defaults
        }
        setTaste(taste);
    }

    public void save() {
        Properties p = new Properties();
        p.setProperty("theme.hue",         String.valueOf(getTaste().getHue()));
        p.setProperty("theme.saturation",  String.valueOf(getTaste().getSaturation()));
        p.setProperty("theme.lightness",   String.valueOf(getTaste().getLightness()));
        p.setProperty("theme.darkMode",    String.valueOf(getTaste().isDarkMode()));
        p.setProperty("theme.accentColor", String.valueOf(getTaste().getAccentBackgroundColor().getRGB()));
        p.setProperty("theme.stripedRows",          String.valueOf(getTaste().isStripedRows()));
        p.setProperty("theme.fillEmptyRows",        String.valueOf(getTaste().isFillEmptyRows()));
        p.setProperty("theme.highContrast",         String.valueOf(getTaste().isHighContrast()));
        p.setProperty("theme.highContrastInverted", String.valueOf(getTaste().isHighContrastInverted()));
        p.setProperty("theme.tintText",             String.valueOf(getTaste().isTintText()));
        if (getTaste().getDesign() != null) {
            p.setProperty("design.id", getTaste().getDesign().getId());
        }
        try {
            Files.createDirectories(CONFIG_FILE.getParent());
            try (OutputStream out = Files.newOutputStream(CONFIG_FILE)) {
                p.store(out, "Spacify theme settings");
            }
        } catch (IOException ignored) {
            // non-fatal — skip silently
        }
    }
}
