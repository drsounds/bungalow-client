package se.spacify.app.localmusic.service;

import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;
import se.spacify.db.DatabaseManager;
import se.spacify.db.LibraryRepository;
import se.spacify.app.localmusic.model.LocalFile;
import se.spacify.app.music.model.Recording;
import se.spacify.app.music.model.RecordingCreatorCredit;
import se.spacify.app.Application;
import se.spacify.service.media.PlaybackSupport;

import se.spacify.app.music.service.MusicService;

import javax.sound.sampled.*;
import javax.swing.*;
import java.io.File;
import java.util.List;

/**
 * JavaSound-based implementation of the music-streaming aspect for local audio
 * files. Supports WAV, AIFF, and AU formats natively; other formats require a
 * SPI codec on the classpath. Local playback needs no sign-in, so this Service
 * implements only {@link MusicService} (no {@link se.spacify.service.AuthAspect}).
 */
public class LocalMusicService implements MusicService {
    private Application plugin;
    public Application getApplication() { return plugin; }
    private final PlaybackSupport playback = new PlaybackSupport();

    private Clip          clip;
    private PlaybackState state        = PlaybackState.IDLE;
    private long          durationMs   = 0;
    private Timer         positionTimer;

    // ── Service identity ──────────────────────────────────────────────────────

    @Override public String getId()   { return "spacify.local.music"; }
    @Override public String getName() { return "Local Music"; }

    // ── Listener registration (delegated to PlaybackSupport) ──────────────────

    @Override public void addPlaybackListener(PlaybackListener l)    { playback.addPlaybackListener(l); }
    @Override public void removePlaybackListener(PlaybackListener l) { playback.removePlaybackListener(l); }

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    @Override
    public void onCreate() {
        // Fire position updates every 500 ms while playing
        positionTimer = new Timer(500, e -> {
            if (clip != null && clip.isRunning())
                playback.firePositionChanged(clip.getMicrosecondPosition() / 1000L, durationMs);
        });
    }

    @Override public void onStart()   {}

    @Override
    public void onStop() {
        stop();
        if (positionTimer != null) positionTimer.stop();
    }

    @Override
    public void onDestroy() {
        if (clip != null) { clip.close(); clip = null; }
    }

    // ── Playback ──────────────────────────────────────────────────────────────

    @Override
    public void play() {
        if (clip == null || state == PlaybackState.PLAYING) return;
        clip.start();
        positionTimer.start();
        setState(PlaybackState.PLAYING);
    }

    @Override
    public void pause() {
        if (clip == null || state != PlaybackState.PLAYING) return;
        clip.stop();
        positionTimer.stop();
        setState(PlaybackState.PAUSED);
    }

    @Override
    public void stop() {
        if (clip == null) return;
        clip.stop();
        clip.setMicrosecondPosition(0);
        positionTimer.stop();
        setState(PlaybackState.STOPPED);
        playback.firePositionChanged(0, durationMs);
    }

    @Override
    public void seek(long positionMs) {
        if (clip == null) return;
        clip.setMicrosecondPosition(positionMs * 1000L);
        playback.firePositionChanged(positionMs, durationMs);
    }

    @Override
    public void loadUri(String uri) {
        if (uri == null) return;
        if (uri.startsWith("spacify:local:")) {
            loadFile(new File(uri.substring("spacify:local:".length())), null, null, null);
        } else {
            // musik:isrc:<ISRC> (canonical) or the legacy spacify:recording:isrc: form.
            String isrc = se.spacify.app.music.net.MusikUri.isrcOf(uri);
            if (isrc != null) loadByIsrc(isrc);
        }
    }

    @Override
    public void loadByIsrc(String isrc) {
        try {
            // Prefer a dedicated local file row — it carries its own metadata.
            List<LocalFile> files = DatabaseManager.getInstance().dao(LocalFile.class)
                .queryForEq("isrc", isrc);
            if (!files.isEmpty()) { loadLocalFile(files.get(0)); return; }

            List<Recording> results = DatabaseManager.getInstance().dao(Recording.class)
                .queryForEq("isrc", isrc);
            if (results.isEmpty()) { playback.fireError(new Exception("No recording found for ISRC: " + isrc)); return; }
            Recording rec = results.get(0);
            String path = LibraryRepository.filePathForRecording(rec);
            if (path == null) { playback.fireError(new Exception("No local file for ISRC: " + isrc)); return; }
            loadFile(new File(path), rec.getName(), primaryArtist(rec), null);
        } catch (Exception e) {
            playback.fireError(e);
        }
    }

    /** Load a local file row for playback, using its denormalised metadata. */
    public void loadLocalFile(LocalFile f) {
        if (f == null || f.getFilePath() == null) {
            playback.fireError(new Exception("Local file has no path"));
            return;
        }
        loadFile(new File(f.getFilePath()), f.getName(), f.getArtistName(), f.getReleaseName());
    }

    @Override
    public void loadByTitleArtist(String title, String artist) {
        Recording match = lookupByTitleArtist(title, artist);
        if (match == null) { playback.fireError(new Exception("Not found: " + title)); return; }
        String path = LibraryRepository.filePathForRecording(match);
        if (path != null) {
            loadFile(new File(path), match.getName(), primaryArtist(match), null);
        } else {
            loadUri(match.getPlayUri());
        }
    }

    @Override
    public Recording lookupByTitleArtist(String title, String artist) {
        if (title == null || title.isBlank()) return null;
        try {
            for (Recording r : DatabaseManager.getInstance().dao(Recording.class).queryForAll()) {
                if (r.getName() == null || !r.getName().equalsIgnoreCase(title)) continue;
                if (artist == null || artist.isBlank()) return r;
                if (primaryArtist(r).equalsIgnoreCase(artist)) return r;
            }
            return null;
        } catch (Exception e) {
            return null;
        }
    }

    @Override
    public Recording lookup(String isrc) {
        try {
            Recording rec = DatabaseManager.getInstance().dao(Recording.class)
                .queryForEq("isrc", isrc).stream().findFirst().orElse(null);
            if (rec != null) return rec;

            // A local file with this ISRC is also playable here — return a
            // lightweight, unpersisted Recording as a "can play" token.
            LocalFile f = DatabaseManager.getInstance().dao(LocalFile.class)
                .queryForEq("isrc", isrc).stream().findFirst().orElse(null);
            if (f != null) {
                Recording token = new Recording(f.getName());
                token.setIsrc(f.getIsrc());
                return token;
            }
            return null;
        } catch (Exception e) { return null; }
    }

    // ── State helpers ─────────────────────────────────────────────────────────

    @Override public PlaybackState getPlaybackState() { return state; }
    @Override public long getPositionMs() { return clip != null ? clip.getMicrosecondPosition() / 1000L : 0; }
    @Override public long getDurationMs() { return durationMs; }

    // ── Private ───────────────────────────────────────────────────────────────

    private void loadFile(File file, String title, String artist, String album) {
        setState(PlaybackState.LOADING);
        try {
            if (clip != null) { clip.stop(); clip.close(); }
            AudioInputStream raw = AudioSystem.getAudioInputStream(file);
            AudioFormat fmt = raw.getFormat();
            // Convert to PCM if needed (e.g. MP3 via SPI)
            if (fmt.getEncoding() != AudioFormat.Encoding.PCM_SIGNED) {
                AudioFormat pcm = new AudioFormat(fmt.getSampleRate(), 16, fmt.getChannels(), true, false);
                raw = AudioSystem.getAudioInputStream(pcm, raw);
            }
            final Clip c = AudioSystem.getClip();
            clip = c;
            c.open(raw);
            durationMs = c.getMicrosecondLength() / 1000L;
            // Detect natural end-of-media: a STOP on the *current* clip while we
            // still believe we're playing and the playhead has reached the end.
            // The c == clip guard ignores STOPs from a clip we've since replaced.
            c.addLineListener(ev -> {
                if (ev.getType() == LineEvent.Type.STOP
                        && c == clip
                        && state == PlaybackState.PLAYING
                        && c.getMicrosecondPosition() >= c.getMicrosecondLength()) {
                    SwingUtilities.invokeLater(this::handleCompletion);
                }
            });
            setState(PlaybackState.PAUSED);

            String t = title  != null ? title  : file.getName();
            String a = artist != null ? artist : "";
            String al = album != null ? album  : "";
            playback.fireTrackChanged(t, a, al);
            playback.firePositionChanged(0, durationMs);
        } catch (Exception e) {
            setState(PlaybackState.ERROR);
            playback.fireError(e);
        }
    }

    /** Natural end-of-track: stop the timer, mark stopped, and notify listeners. */
    private void handleCompletion() {
        if (positionTimer != null) positionTimer.stop();
        setState(PlaybackState.STOPPED);
        playback.firePositionChanged(durationMs, durationMs);
        playback.fireCompleted();
    }

    private void setState(PlaybackState s) {
        state = s;
        playback.fireStateChanged(s);
    }

    private String primaryArtist(Recording rec) {
        try {
            return DatabaseManager.getInstance().dao(RecordingCreatorCredit.class)
                .queryForEq("recording_id", rec.getId()).stream()
                .filter(RecordingCreatorCredit::isPrimary)
                .findFirst()
                .map(c -> c.getArtist().getName())
                .orElse("");
        } catch (Exception e) { return ""; }
    }
    
    @Override
    public void onRegister(AspectManager<? extends Aspect> aspectManager) {
        // TODO Auto-generated method stub
    }
}
