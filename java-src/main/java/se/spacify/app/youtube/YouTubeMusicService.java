package se.spacify.app.youtube;

import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;
import se.spacify.app.music.model.Recording;
import se.spacify.app.ApplicationSettings;
import se.spacify.app.media.service.MediaServicePlayerComponent;
import se.spacify.app.music.service.MusicService;
import se.spacify.service.media.PlaybackSupport;

/**
 * Music-streaming Service backed by YouTube. Playback runs in a
 * {@link YouTubePlayerComponent} (an embedded IFrame-API player); this Service
 * resolves tracks by title/artist search and drives that component.
 *
 * <p>It is a deliberate <em>fallback</em>: {@link #lookup(String)} can't resolve
 * an ISRC (returns null), but {@link #lookupByTitleArtist} reports any non-blank
 * title as playable, so — registered after the local Service — it catches tracks
 * nothing else can play. The shared {@code PlaybackCoordinator} then marks it the
 * active Service and the Now Playing panel shows its player component.
 */
public class YouTubeMusicService implements MusicService {

    /** Settings key for the optional YouTube Data API key. */
    public static final String SETTING_API_KEY = "apiKey";

    private final PlaybackSupport playback = new PlaybackSupport();
    private final ApplicationSettings settings;

    public YouTubeMusicService() { this(null); }
    public YouTubeMusicService(ApplicationSettings settings) { this.settings = settings; }

    /** The configured YouTube Data API key, or "" to use the no-key scrape. */
    String apiKey() {
        return settings != null ? settings.getString(SETTING_API_KEY) : "";
    }

    private YouTubePlayerComponent player;
    private PlaybackState state      = PlaybackState.IDLE;
    private long          positionMs = 0;
    private long          durationMs = 0;

    // ── Identity ────────────────────────────────────────────────────────────────

    @Override public String getId()   { return "se.spacify.app.youtube"; }
    @Override public String getName() { return "YouTube"; }

    // ── Listener registration ─────────────────────────────────────────────────

    @Override public void addPlaybackListener(PlaybackListener l)    { playback.addPlaybackListener(l); }
    @Override public void removePlaybackListener(PlaybackListener l) { playback.removePlaybackListener(l); }

    // ── Player component ────────────────────────────────────────────────────────

    /** Lazily create the embedded player (on the EDT, where playback is triggered). */
    private YouTubePlayerComponent player() {
        if (player == null) player = new YouTubePlayerComponent(this);
        return player;
    }

    @Override
    public MediaServicePlayerComponent getPlayerComponent() { return player(); }

    // ── Resolution (fallback semantics) ─────────────────────────────────────────

    @Override
    public Recording lookup(String isrc) { return null; }   // no ISRC mapping

    @Override
    public Recording lookupByTitleArtist(String title, String artist) {
        if (title == null || title.isBlank()) return null;
        // A non-null token signals "YouTube can play this" to the coordinator.
        Recording token = new Recording(title);
        return token;
    }

    /**
     * Offer the top YouTube search results as concrete candidates for the
     * "Play with…" chooser, each a specific video — so the user can pick the right
     * one and it plays exactly that, rather than re-searching to the first hit.
     */
    @Override
    public java.util.List<Candidate> findCandidates(String isrc, String title, String artist) {
        if (title == null || title.isBlank()) return java.util.List.of();
        String query = (artist == null || artist.isBlank()) ? title : title + " " + artist;
        java.util.List<Candidate> out = new java.util.ArrayList<>();
        for (YouTubeSearch.Result r : YouTubeSearch.search(query, apiKey())) {
            out.add(new Candidate("spacify:youtube:" + r.videoId(), r.title(), artist, false));
        }
        return out;
    }

    // ── Loading ─────────────────────────────────────────────────────────────────

    @Override
    public void loadByIsrc(String isrc) {
        // YouTube can't resolve a bare ISRC; the coordinator never routes one here
        // (lookup returns null), so treat a direct call as an error.
        playback.fireError(new UnsupportedOperationException("YouTube cannot load by ISRC: " + isrc));
    }

    @Override
    public void loadByTitleArtist(String title, String artist) {
        String query = artist == null || artist.isBlank() ? title : title + " " + artist;
        setState(PlaybackState.LOADING);
        player().loadAndPlay(query);
    }

    @Override
    public void loadUri(String uri) {
        if (uri == null) return;
        if (uri.startsWith("spacify:youtube:")) {
            // Treat the remainder as a search query (or a video id YouTube resolves).
            setState(PlaybackState.LOADING);
            player().loadAndPlay(uri.substring("spacify:youtube:".length()));
        }
    }

    // ── Transport (delegated to the component) ──────────────────────────────────

    @Override public void play()  { player().play(); }
    @Override public void pause() { player().pause(); }

    @Override
    public void stop() {
        player().pause();
        positionMs = 0;
        setState(PlaybackState.STOPPED);
    }

    @Override public void seek(long positionMs) { player().seekTo(positionMs); }

    @Override public PlaybackState getPlaybackState() { return state; }
    @Override public long getPositionMs() { return positionMs; }
    @Override public long getDurationMs() { return durationMs; }

    // ── Callbacks from the player component ──────────────────────────────────────

    /** Map a YouTube IFrame player state to ours and notify listeners. */
    void onPlayerState(int ytState) {
        switch (ytState) {
            case 1 -> setState(PlaybackState.PLAYING);   // playing
            case 2 -> setState(PlaybackState.PAUSED);    // paused
            case 3 -> setState(PlaybackState.LOADING);   // buffering
            case 0 -> {                                  // ended
                setState(PlaybackState.STOPPED);
                playback.fireCompleted();
            }
            default -> { /* -1 unstarted, 5 cued: leave state as-is */ }
        }
    }

    void onPlayerTime(double currentSec, double durationSec) {
        positionMs = (long) (currentSec * 1000);
        durationMs = (long) (durationSec * 1000);
        playback.firePositionChanged(positionMs, durationMs);
    }

    void onPlayerTitle(String title) {
        playback.fireTrackChanged(title, "", "");
    }

    private void setState(PlaybackState s) {
        state = s;
        playback.fireStateChanged(s);
    } 
    @Override
    public void onRegister(AspectManager<? extends Aspect> aspectManager) {
        // TODO Auto-generated method stub
    }
}
