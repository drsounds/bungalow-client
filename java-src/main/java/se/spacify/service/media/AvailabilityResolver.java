package se.spacify.service.media;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import javax.swing.SwingUtilities;

import se.spacify.library.LibraryEvents;
import se.spacify.app.music.service.MusicService;
import se.spacify.ui.MainWindow;

/**
 * Resolves {@link TrackAvailability} for a {@link PlayRequest} with a cheap,
 * non-network probe — asking each {@link MusicService} whether it can resolve the
 * track via {@code lookup}/{@code lookupByTitleArtist} (local = DB; YouTube =
 * a token). Results are cached by {@link PlayRequest#key()} and computed on a
 * single background thread with a short debounce, so a fast scroll over the
 * Buy/Stream column doesn't storm the services. The cache is cleared when the
 * library changes.
 */
public final class AvailabilityResolver {

    /** The local-playback service id; matches there mean "have a local file". */
    private static final String LOCAL_SERVICE_ID = "spacify.local.music";
    private static final long DEBOUNCE_MS = 150;

    private static final AvailabilityResolver INSTANCE = new AvailabilityResolver();
    public static AvailabilityResolver get() { return INSTANCE; }

    private final Map<String, TrackAvailability> cache = new ConcurrentHashMap<>();
    private final Set<String> inFlight = ConcurrentHashMap.newKeySet();
    private final ScheduledExecutorService exec = Executors.newSingleThreadScheduledExecutor(r -> {
        Thread t = new Thread(r, "availability-resolver");
        t.setDaemon(true);
        return t;
    });

    private AvailabilityResolver() {
        // Availability (esp. local) changes when the library is scanned/edited.
        LibraryEvents.addListener(cache::clear);
    }

    /**
     * The cached availability, or {@link TrackAvailability#PENDING} while it
     * resolves in the background. {@code onResolved} runs on the EDT once ready
     * (e.g. to repaint the table). Safe to call from a cell renderer.
     */
    public TrackAvailability availabilityFor(PlayRequest req, Runnable onResolved) {
        if (req == null) return TrackAvailability.PENDING;
        String key = req.key();
        if (key == null) return TrackAvailability.PENDING;

        TrackAvailability cached = cache.get(key);
        if (cached != null) return cached;

        if (inFlight.add(key)) {
            exec.schedule(() -> {
                try {
                    cache.put(key, probe(req));
                } finally {
                    inFlight.remove(key);
                }
                if (onResolved != null) SwingUtilities.invokeLater(onResolved);
            }, DEBOUNCE_MS, TimeUnit.MILLISECONDS);
        }
        return TrackAvailability.PENDING;
    }

    private TrackAvailability probe(PlayRequest req) {
        MainWindow mw = MainWindow.getInstance();
        if (mw == null) return TrackAvailability.PENDING;

        boolean local = false;
        List<MusicService> stream = new ArrayList<>();
        for (MusicService ms : mw.getServiceManager().getServices(MusicService.class)) {
            if (!canResolve(ms, req)) continue;
            if (LOCAL_SERVICE_ID.equals(ms.getId())) local = true;
            else stream.add(ms);
        }
        return new TrackAvailability(true, local, stream);
    }

    private static boolean canResolve(MusicService ms, PlayRequest req) {
        try {
            if (req.isrc() != null && ms.lookup(req.isrc()) != null) return true;
            return req.title() != null && !req.title().isBlank()
                && ms.lookupByTitleArtist(req.title(), req.artist()) != null;
        } catch (Exception e) {
            return false;
        }
    }
}
