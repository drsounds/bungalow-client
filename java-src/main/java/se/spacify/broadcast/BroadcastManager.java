package se.spacify.broadcast;

import javax.swing.SwingUtilities;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * Process-wide publish/subscribe bus for cross-plugin communication — an
 * Android-style local broadcast manager. Any component (a plugin view, a menu
 * item, an in-app header spinner, …) can {@link #register} a
 * {@link BroadcastReceiver} for an action and react to {@link Broadcast}s sent by
 * any other part of the app, without a direct dependency between them.
 *
 * <p>Register with the wildcard action {@code "*"} to receive every broadcast.
 * Delivery always happens on the EDT, so receivers may update Swing directly.
 */
public final class BroadcastManager {

    /** Wildcard action: a receiver registered for this gets every broadcast. */
    public static final String ALL = "*";

    private static final BroadcastManager INSTANCE = new BroadcastManager();
    public static BroadcastManager get() { return INSTANCE; }

    private BroadcastManager() {}

    private final Map<String, List<BroadcastReceiver>> receivers = new ConcurrentHashMap<>();

    /** Subscribe {@code receiver} to {@code action} (or {@link #ALL} for everything). */
    public void register(String action, BroadcastReceiver receiver) {
        receivers.computeIfAbsent(action, a -> new CopyOnWriteArrayList<>()).add(receiver);
    }

    /** Remove {@code receiver} from every action it was registered for. */
    public void unregister(BroadcastReceiver receiver) {
        for (List<BroadcastReceiver> list : receivers.values()) list.remove(receiver);
    }

    /** Send a broadcast; receivers for its action and for {@link #ALL} are notified on the EDT. */
    public void send(Broadcast broadcast) {
        if (SwingUtilities.isEventDispatchThread()) {
            deliver(broadcast);
        } else {
            SwingUtilities.invokeLater(() -> deliver(broadcast));
        }
    }

    /** Convenience for a no-extras broadcast. */
    public void send(String action) {
        send(Broadcast.of(action));
    }

    private void deliver(Broadcast broadcast) {
        notifyList(receivers.get(broadcast.action()), broadcast);
        notifyList(receivers.get(ALL), broadcast);
    }

    private void notifyList(List<BroadcastReceiver> list, Broadcast broadcast) {
        if (list == null) return;
        for (BroadcastReceiver r : new ArrayList<>(list)) {
            try {
                r.onReceive(broadcast);
            } catch (Exception e) {
                System.err.println("Broadcast receiver failed for " + broadcast.action() + ": " + e);
            }
        }
    }
}
