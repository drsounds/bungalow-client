package se.spacify.broadcast;

/**
 * Receives {@link Broadcast}s for the action(s) it was registered against — the
 * equivalent of an Android {@code BroadcastReceiver}. Always invoked on the EDT,
 * so implementations may touch Swing directly.
 */
@FunctionalInterface
public interface BroadcastReceiver {
    void onReceive(Broadcast broadcast);
}
