package se.spacify.ui;

import java.awt.Color;

import javax.swing.JLabel;
import javax.swing.Timer;

import se.spacify.broadcast.BroadcastManager;

/**
 * A small animated spinner that shows how many downloads are in progress. It is
 * a pure consumer of the broadcast bus: it has <em>no</em> dependency on the
 * downloads plugin and simply listens for the {@code bungalow.downloads.changed}
 * action (the documented contract published by
 * {@code se.spacify.app.downloads.DownloadActions#CHANGED}). This is the
 * cross-plugin reporting the broadcast system enables — the same pattern works
 * for menu items, in-app headers, etc.
 */
public class DownloadActivityIndicator extends JLabel {

    private static final long serialVersionUID = 1L;

    /** Broadcast contract — kept in sync with DownloadActions.CHANGED. */
    private static final String ACTION = "bungalow.downloads.changed";
    private static final String[] SPINNER = {"⠋","⠙","⠹","⠸","⠼","⠴","⠦","⠧","⠇","⠏"};

    private int active = 0;
    private int frame = 0;
    private final Timer timer;

    public DownloadActivityIndicator() {
        setForeground(new Color(170, 170, 170));
        setVisible(false);
        timer = new Timer(120, e -> { frame = (frame + 1) % SPINNER.length; render(); });
        BroadcastManager.get().register(ACTION, b -> {
            active = b.getInt("active", 0);
            if (active > 0 && !timer.isRunning()) timer.start();
            else if (active <= 0 && timer.isRunning()) timer.stop();
            setVisible(active > 0);
            render();
        });
    }

    private void render() {
        if (active > 0) {
            setText(SPINNER[frame] + "  " + active + (active == 1 ? " download" : " downloads"));
        }
    }
}
