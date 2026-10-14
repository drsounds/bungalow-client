package se.spacify.app;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Properties;

/**
 * Typed, persisted setting values for one plugin, backed by
 * {@code ~/.spacify/plugins/<id>.properties}. Reads fall back to the schema's
 * declared defaults. Arrays are stored as newline-separated values in a single
 * property (java.util.Properties round-trips embedded newlines via {@code \n}).
 */
public final class ApplicationSettings {

    private final String pluginId;
    private final Map<String, ApplicationSetting> schema = new LinkedHashMap<>();
    private final Properties props = new Properties();

    ApplicationSettings(String pluginId, List<ApplicationSetting> schema) {
        this.pluginId = pluginId;
        for (ApplicationSetting s : schema) this.schema.put(s.getKey(), s);
        load();
    }

    // ── Typed accessors ─────────────────────────────────────────────────────

    public boolean getBool(String key) {
        String v = props.getProperty(key);
        if (v != null) return Boolean.parseBoolean(v);
        Object d = defaultOf(key);
        return d instanceof Boolean b && b;
    }

    public String getString(String key) {
        String v = props.getProperty(key);
        if (v != null) return v;
        Object d = defaultOf(key);
        return d instanceof String s ? s : "";
    }

    public int getInt(String key) {
        String v = props.getProperty(key);
        if (v != null) try { return Integer.parseInt(v.trim()); } catch (NumberFormatException ignored) {}
        Object d = defaultOf(key);
        return d instanceof Integer i ? i : 0;
    }

    public String[] getStringArray(String key) {
        String v = props.getProperty(key);
        if (v != null) return splitLines(v);
        Object d = defaultOf(key);
        return d instanceof String[] a ? a.clone() : new String[0];
    }

    public int[] getIntArray(String key) {
        String v = props.getProperty(key);
        if (v != null) {
            String[] parts = splitLines(v);
            List<Integer> out = new ArrayList<>();
            for (String p : parts) try { out.add(Integer.parseInt(p.trim())); } catch (NumberFormatException ignored) {}
            int[] r = new int[out.size()];
            for (int i = 0; i < r.length; i++) r[i] = out.get(i);
            return r;
        }
        Object d = defaultOf(key);
        return d instanceof int[] a ? a.clone() : new int[0];
    }

    // ── Mutation ────────────────────────────────────────────────────────────

    /** Set a value; {@code value} type should match the key's declared {@link ApplicationSetting.Type}. */
    public void set(String key, Object value) {
        if (value instanceof String[] sa) {
            props.setProperty(key, String.join("\n", sa));
        } else if (value instanceof int[] ia) {
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < ia.length; i++) { if (i > 0) sb.append('\n'); sb.append(ia[i]); }
            props.setProperty(key, sb.toString());
        } else {
            props.setProperty(key, String.valueOf(value));
        }
    }

    // ── Persistence ─────────────────────────────────────────────────────────

    private void load() {
        java.nio.file.Path f = ApplicationPaths.settingsFile(pluginId);
        if (!Files.exists(f)) return;
        try (InputStream in = Files.newInputStream(f)) {
            props.load(in);
        } catch (IOException ignored) {}
    }

    public void save() {
        try {
            Files.createDirectories(ApplicationPaths.stateDir());
            try (OutputStream out = Files.newOutputStream(ApplicationPaths.settingsFile(pluginId))) {
                props.store(out, "Spacify plugin settings: " + pluginId);
            }
        } catch (IOException ignored) {}
    }

    // ── Helpers ─────────────────────────────────────────────────────────────

    private Object defaultOf(String key) {
        ApplicationSetting s = schema.get(key);
        return s != null ? s.getDefaultValue() : null;
    }

    private static String[] splitLines(String v) {
        if (v.isEmpty()) return new String[0];
        return v.split("\n", -1);
    }
}
