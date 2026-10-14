package se.spacify.broadcast;

import java.util.HashMap;
import java.util.Map;

/**
 * An immutable broadcast message — the equivalent of an Android {@code Intent}.
 * Carries an {@code action} string that receivers filter on, plus arbitrary
 * {@code extras}. Build with {@link #of} and chain {@link #with} to attach data.
 *
 * <pre>{@code
 *   BroadcastManager.get().send(
 *       Broadcast.of(DownloadActions.CHANGED).with("active", 2));
 * }</pre>
 */
public final class Broadcast {

    private final String action;
    private final Map<String, Object> extras;

    private Broadcast(String action, Map<String, Object> extras) {
        this.action = action;
        this.extras = extras;
    }

    public static Broadcast of(String action) {
        return new Broadcast(action, new HashMap<>());
    }

    /** Return a copy with an extra attached (this instance stays unchanged-ish; cheap copy). */
    public Broadcast with(String key, Object value) {
        Map<String, Object> copy = new HashMap<>(extras);
        copy.put(key, value);
        return new Broadcast(action, copy);
    }

    public String action() { return action; }

    public Object get(String key) { return extras.get(key); }

    public Object get(String key, Object defaultValue) {
        Object v = extras.get(key);
        return v != null ? v : defaultValue;
    }

    public int getInt(String key, int defaultValue) {
        Object v = extras.get(key);
        return v instanceof Number n ? n.intValue() : defaultValue;
    }

    public long getLong(String key, long defaultValue) {
        Object v = extras.get(key);
        return v instanceof Number n ? n.longValue() : defaultValue;
    }

    public String getString(String key) {
        Object v = extras.get(key);
        return v != null ? String.valueOf(v) : null;
    }

    @Override public String toString() { return "Broadcast[" + action + " " + extras + "]"; }
}
