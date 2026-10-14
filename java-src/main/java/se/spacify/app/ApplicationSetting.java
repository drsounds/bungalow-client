package se.spacify.app;

/**
 * Declaration of one configurable plugin setting: a stable {@code key}, a
 * human {@code label}, a {@link Type}, and a default value. The plugin manager
 * renders an editor per type (checkbox / text field / spinner / multi-line list)
 * and persists values via {@link ApplicationSettings}.
 */
public final class ApplicationSetting {

    public enum Type { BOOL, STRING, INT, STRING_ARRAY, INT_ARRAY }

    private final String key;
    private final String label;
    private final Type   type;
    private final Object defaultValue;

    private ApplicationSetting(String key, String label, Type type, Object defaultValue) {
        this.key = key;
        this.label = label;
        this.type = type;
        this.defaultValue = defaultValue;
    }

    public static ApplicationSetting bool(String key, String label, boolean def) {
        return new ApplicationSetting(key, label, Type.BOOL, def);
    }

    public static ApplicationSetting string(String key, String label, String def) {
        return new ApplicationSetting(key, label, Type.STRING, def == null ? "" : def);
    }

    public static ApplicationSetting integer(String key, String label, int def) {
        return new ApplicationSetting(key, label, Type.INT, def);
    }

    public static ApplicationSetting stringArray(String key, String label, String[] def) {
        return new ApplicationSetting(key, label, Type.STRING_ARRAY, def == null ? new String[0] : def);
    }

    public static ApplicationSetting intArray(String key, String label, int[] def) {
        return new ApplicationSetting(key, label, Type.INT_ARRAY, def == null ? new int[0] : def);
    }

    public String getKey()          { return key; }
    public String getLabel()        { return label; }
    public Type   getType()         { return type; }
    public Object getDefaultValue() { return defaultValue; }
}
