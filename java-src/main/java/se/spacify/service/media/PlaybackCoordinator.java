package se.spacify.service.media;

import se.spacify.app.localmusic.model.LocalFile;
import se.spacify.app.media.model.MusicServiceTrack;

import se.spacify.app.localmusic.service.LocalMusicService;
import se.spacify.app.media.service.MediaService;
import se.spacify.app.music.service.MusicService;
import se.spacify.ui.MainWindow;
import se.spacify.ui.ServiceMatchDialog;

import javax.swing.JOptionPane;
import javax.swing.SwingWorker;
import java.util.ArrayList;
import java.util.List;
import java.util.function.Consumer;

/**
 * Routes a library "play" request to a registered Service. It first asks every
 * {@link MusicService} to resolve the row by ISRC ({@link MusicService#lookup}),
 * then falls back to a title/artist metadata lookup
 * ({@link MusicService#lookupByTitleArtist}); the first Service that resolves
 * the request plays it. Also supports direct URI / local-file playback.
 *
 * <p>The Service that handled the most recent play is the <em>active</em> Service
 * ({@link #getActiveService()}); listeners ({@link #addActiveServiceListener})
 * are notified when it changes, so UI (the Now Playing panel, the footer) can
 * switch to that Service's player component and transport target.
 */
public final class PlaybackCoordinator {

    private MainWindow mainWindow;

    public static MainWindow staticMainWindow;

    public MainWindow getMainWindow() {
        return mainWindow;
    }

    private PlaybackCoordinator(MainWindow mainWindow) {
        this.mainWindow = mainWindow;
        staticMainWindow = mainWindow;
    }

    /**
     * Wire the coordinator to the live window. Must be called once during start-up:
     * the static play/resolve entry points reach the Services through this window,
     * and without it {@link #resolveAndPlay} can't gather matches (so the
     * "Play with…" resolution dialog never appears).
     */
    public static void init(MainWindow mainWindow) {
        staticMainWindow = mainWindow;
    }

    private static MediaService activeService;
    private static final List<Consumer<MediaService>> activeListeners = new ArrayList<>();

    /** The Service that handled the most recent play, or null if none yet. */
    public static MediaService getActiveService() { return activeService; }

    /** Subscribe to active-Service changes (fired on the calling/EDT thread). */
    public static void addActiveServiceListener(Consumer<MediaService> l) { activeListeners.add(l); }

    /** Record the active Service and notify listeners if it changed. */
    private static void setActiveService(MediaService s) {
        if (s == activeService) return;
        activeService = s;
        for (Consumer<MediaService> l : List.copyOf(activeListeners)) l.accept(s);
    }

    /**
     * Resolve a track across all registered Services, by ISRC first and then by
     * title/artist metadata, playing it on the first Service that resolves it.
     *
     * @return true if a Service handled the request.
     */
    public static boolean play(String isrc, String title, String artist) {
        return playByIsrc(isrc) || playByMetadata(title, artist);
    }

    /**
     * Look up the ISRC across all registered music Services and play it on the
     * first that resolves it.
     *
     * @return true if a Service handled the request.
     */
    public static boolean playByIsrc(String isrc) {
        if (isrc == null || isrc.isBlank()) return false;
        for (MusicService ms : staticMainWindow.getServiceManager().getServices(MusicService.class)) {
            if (ms.lookup(isrc) != null) {
                setActiveService(ms);
                ms.loadByIsrc(isrc);
                ms.play();
                return true;
            }
        }
        return false;
    }

    /**
     * Look up by title/artist metadata across all registered music Services and
     * play it on the first that resolves it.
     *
     * @return true if a Service handled the request.
     */
    public static boolean playByMetadata(String title, String artist) {
        if (title == null || title.isBlank()) return false;
        for (MusicService ms : staticMainWindow.getServiceManager().getServices(MusicService.class)) {
            if (ms.lookupByTitleArtist(title, artist) != null) {
                setActiveService(ms);
                ms.loadByTitleArtist(title, artist);
                ms.play();
                return true;
            }
        }
        return false;
    }

    /** Play a request directly on a specific Service (the Stream submenu quick-play). */
    public static void playOn(MusicService ms, PlayRequest req) {
        if (ms == null || req == null) return;
        setActiveService(ms);
        if (req.isrc() != null && !req.isrc().isBlank()) ms.loadByIsrc(req.isrc());
        else ms.loadByTitleArtist(req.title(), req.artist());
        ms.play();
    }

    /** Play an arbitrary content URI, routing by scheme to the Service that handles it. */
    public static boolean playUri(String uri) {
        if (uri == null) return false;
        // A musik: (or legacy spacify:recording:isrc:) URI names portable identity,
        // not a vendor target: resolve it to a PlayRequest and play by identifier /
        // metadata across the installed Services (RFC-0002 §8).
        if (se.spacify.app.music.net.MusikUri.isContentUri(uri)) {
            PlayRequest req = se.spacify.app.music.net.MusikUri.toPlayRequest(uri);
            if (req == null) return false;
            if (req.isrc() != null && playByIsrc(req.isrc())) return true;
            return playByMetadata(req.title(), req.artist());
        }
        // YouTube URIs must play on the YouTube service, not the first media one.
        if (uri.startsWith("spacify:youtube:")) {
            MusicService yt = findService("se.spacify.app.youtube");
            if (yt != null) {
                setActiveService(yt);
                yt.loadUri(uri);
                yt.play();
                return true;
            }
        }
        MediaService ms = staticMainWindow.getServiceManager().getService(MediaService.class);
        if (ms == null) return false;
        setActiveService(ms);
        ms.loadUri(uri);
        ms.play();
        return true;
    }

    /** Play a local file directly via the local music Service. */
    public static boolean playLocalFile(LocalFile file) {
        LocalMusicService local = staticMainWindow.getServiceManager().getService(LocalMusicService.class);
        if (local == null || file == null) return false;
        setActiveService(local);
        local.loadLocalFile(file);
        local.play();
        return true;
    }

    // ── "Play with…" resolution ─────────────────────────────────────────────────

    /**
     * Play {@code req}, honouring the user's saved "Play with…" choice. If a saved
     * {@link MusicServiceTrack} exists for this track it plays straight away on
     * that Service; otherwise the track is "unresolved" and the cross-Service
     * matches are gathered (off the EDT) and offered in the
     * {@link ServiceMatchDialog} so the user can choose — optionally remembering
     * the pick. Call on the EDT.
     */
    public static void resolveAndPlay(PlayRequest req) {
        resolveAndPlay(req, false);
    }

    /**
     * As {@link #resolveAndPlay(PlayRequest)}, but {@code forceChooser} skips any
     * saved choice and always shows the chooser — the "Play with…" / re-lookup
     * action behind the right-click menu.
     */
    public static void resolveAndPlay(PlayRequest req, boolean forceChooser) {
        if (req == null) return;
        if (!forceChooser) {
            MusicServiceTrack saved = ResolutionStore.find(req);
            if (saved != null && playSaved(saved)) return;
        }
        gatherAndChoose(req);
    }

    /** Replay a saved pick on its Service; false if that Service is gone. */
    private static boolean playSaved(MusicServiceTrack saved) {
        MusicService ms = findService(saved.getServiceId());
        if (ms == null) return false;
        setActiveService(ms);
        // Prefer the concrete saved URI (e.g. the exact YouTube video) so the pick
        // replays precisely instead of re-searching by title.
        if (notBlank(saved.getMatchUri())) {
            ms.loadUri(saved.getMatchUri());
        } else if (notBlank(saved.getMatchIsrc())) {
            ms.loadByIsrc(saved.getMatchIsrc());
        } else if (notBlank(saved.getMatchTitle())) {
            ms.loadByTitleArtist(saved.getMatchTitle(), saved.getMatchArtist());
        } else {
            return false;
        }
        ms.play();
        return true;
    }

    /** Gather matches in the background, then offer the chooser (and play the pick). */
    private static void gatherAndChoose(PlayRequest req) {
        new SwingWorker<List<ServiceMatch>, Void>() {
            @Override protected List<ServiceMatch> doInBackground() { return gatherMatches(req); }

            @Override protected void done() {
                List<ServiceMatch> matches;
                try { matches = get(); } catch (Exception e) { matches = List.of(); }

                if (matches.isEmpty()) {
                    if (req.fallbackUri() != null) {
                        playUri(req.fallbackUri());
                    } else {
                        JOptionPane.showMessageDialog(null,
                            "No installed Service can play \"" + req.title() + "\".",
                            "Couldn't play track", JOptionPane.INFORMATION_MESSAGE);
                    }
                    return;
                }

                String label = req.title() + (req.artist().isBlank() ? "" : " — " + req.artist());
                ServiceMatchDialog.Result result = ServiceMatchDialog.choose(null, label, matches);
                if (result == null) return;          // cancelled
                playMatch(result.match(), req);
                if (result.always()) ResolutionStore.save(req, result.match());
            }
        }.execute();
    }

    /**
     * Ask every registered {@link MusicService} whether it can resolve {@code req}
     * — by ISRC first, then by title/artist — collecting one match per Service.
     * Performs blocking network I/O; never call on the EDT.
     */
    public static List<ServiceMatch> gatherMatches(PlayRequest req) {
        List<ServiceMatch> matches = new ArrayList<>();
        for (MusicService ms : staticMainWindow.getServiceManager().getServices(MusicService.class)) {
            // Each Service contributes one or more concrete candidates (e.g. YouTube
            // returns one per search result), all listed in the chooser.
            for (MusicService.Candidate c : ms.findCandidates(req.isrc(), req.title(), req.artist())) {
                matches.add(new ServiceMatch(ms, c));
            }
        }
        return matches;
    }

    /** Load and play a chosen match on its Service. */
    private static void playMatch(ServiceMatch match, PlayRequest req) {
        MusicService ms = match.Service();
        setActiveService(ms);
        if (notBlank(match.uri())) {
            ms.loadUri(match.uri());                          // exact candidate (e.g. a specific video)
        } else if (match.byIsrc() && req.isrc() != null) {
            ms.loadByIsrc(req.isrc());
        } else {
            ms.loadByTitleArtist(req.title(), req.artist());
        }
        ms.play();
    }

    private static MusicService findService(String ServiceId) {
        if (ServiceId == null) return null;
        for (MusicService ms : staticMainWindow.getServiceManager().getServices(MusicService.class)) {
            if (ServiceId.equals(ms.getId())) return ms;
        }
        return null;
    }

    private static boolean notBlank(String s) {
        return s != null && !s.isBlank();
    }
}
