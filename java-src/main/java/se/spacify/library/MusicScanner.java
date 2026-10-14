package se.spacify.library;

import com.j256.ormlite.dao.Dao;
import org.jaudiotagger.audio.AudioFile;
import org.jaudiotagger.audio.AudioFileIO;
import org.jaudiotagger.tag.FieldKey;
import org.jaudiotagger.tag.Tag;
import se.spacify.db.DatabaseManager;
import se.spacify.db.LibraryRepository;
import se.spacify.app.music.model.*;
import se.spacify.app.localmusic.model.LocalFile;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Recursively scans a folder for MP3 files and imports them into the library.
 * For each file it reads ID3 metadata via jaudiotagger and upserts a
 * {@link LocalFile} plus the generic catalogue graph
 * (Artist → Release → Recording → Track), de-duplicating by name/ISRC so
 * re-scanning the same folder does not create duplicates.
 */
public class MusicScanner {

    static {
        // jaudiotagger is extremely chatty on java.util.logging — quiet it down.
        Logger.getLogger("org.jaudiotagger").setLevel(Level.OFF);
    }

    /** Receives progress updates on a background thread. */
    public interface ProgressCallback {
        void onProgress(int done, int total, String currentFile);
        default boolean isCancelled() { return false; }
    }

    private DatabaseManager db() { return DatabaseManager.getInstance(); }

    public ScanResult scan(File folder, ProgressCallback cb) {
        ScanResult result = new ScanResult();
        List<File> files = new ArrayList<>();
        collectMp3s(folder, files);

        int done = 0;
        for (File file : files) {
            if (cb != null && cb.isCancelled()) break;
            if (cb != null) cb.onProgress(done, files.size(), file.getName());
            try {
                importFile(file, result);
                result.filesScanned++;
            } catch (Exception e) {
                result.errors.add(file.getName() + " — " + e.getMessage());
            }
            done++;
        }
        if (cb != null) cb.onProgress(done, files.size(), "");
        return result;
    }

    // ── File discovery ──────────────────────────────────────────────────────────

    private void collectMp3s(File dir, List<File> out) {
        File[] children = dir.listFiles();
        if (children == null) return;
        for (File f : children) {
            if (f.isDirectory()) collectMp3s(f, out);
            else if (f.getName().toLowerCase().endsWith(".mp3")) out.add(f);
        }
    }

    // ── Import one file ───────────────────────────────────────────────────────────

    /** Tagged metadata of an imported file, returned to the caller for display. */
    public record Imported(String title, String artist, String album) {}

    /**
     * Import a single audio file (any jaudiotagger-supported format) into the
     * library, creating/de-duplicating the Artist → Release → Recording → Track →
     * LocalFile graph. Used by the download manager when a download completes.
     */
    public Imported importFile(File file) throws Exception {
        return importFile(file, new ScanResult());
    }

    private Imported importFile(File file, ScanResult result) throws Exception {
        AudioFile audio = AudioFileIO.read(file);
        Tag tag = audio.getTag();

        String fileName    = file.getName();
        String title       = tagOr(tag, FieldKey.TITLE,        stripExtension(fileName));
        String artistName  = tagOr(tag, FieldKey.ARTIST,       "Unknown Artist");
        String albumArtist = tagOr(tag, FieldKey.ALBUM_ARTIST, artistName);
        String albumTitle  = tagOr(tag, FieldKey.ALBUM,        "Unknown Album");
        int    trackNumber = parseTrackNumber(tag != null ? tag.getFirst(FieldKey.TRACK) : null);
        long   durationMs  = audio.getAudioHeader() != null
                                ? audio.getAudioHeader().getTrackLength() * 1000L : 0L;

        String rawIsrc = tag != null ? tag.getFirst(FieldKey.ISRC) : null;
        String isrc = (rawIsrc != null && !rawIsrc.isBlank())
                        ? rawIsrc.trim()
                        : "SPLOCAL-" + Math.abs(file.getAbsolutePath().hashCode());

        Artist    artist    = getOrCreateArtist(artistName, result);
        MusicRelease   release   = getOrCreateRelease(albumTitle, albumArtist, result);
        Recording recording = getOrCreateRecording(isrc, title, file.getAbsolutePath(), durationMs, artist, result);
        getOrCreateTrack(recording, release, trackNumber, durationMs, result);
        getOrCreateLocalFile(isrc, title, artistName, albumTitle, file.getAbsolutePath(), result);
        return new Imported(title, artistName, albumTitle);
    }

    // ── Upsert helpers (count only newly created rows) ─────────────────────────────

    private Artist getOrCreateArtist(String name, ScanResult result) throws Exception {
        List<Artist> existing = db().dao(Artist.class).queryForEq("name", name);
        if (!existing.isEmpty()) return existing.get(0);
        Artist a = new Artist(name);
        db().dao(Artist.class).create(a);
        result.artistsAdded++;
        return a;
    }

    private MusicRelease getOrCreateRelease(String title, String albumArtist, ScanResult result) throws Exception {
        Dao<MusicRelease, Integer> dao = db().dao(MusicRelease.class);
        for (MusicRelease r : dao.queryForEq("title", title)) {
            // Same title AND same album artist counts as the same release, so we
            // don't merge unrelated albums that happen to share a title.
            if (albumArtist == null || albumArtist.isBlank()
                    || LibraryRepository.artistNamesForRelease(r).equalsIgnoreCase(albumArtist.trim())) {
                return r;
            }
        }
        MusicRelease r = new MusicRelease(title);
        r.setType(MusicRelease.ReleaseType.ALBUM);
        dao.create(r);
        result.releasesAdded++;
        if (albumArtist != null && !albumArtist.isBlank()) {
            Artist a = getOrCreateArtist(albumArtist, result);
            db().dao(ReleaseCreatorCredit.class).create(new ReleaseCreatorCredit(r, a, true, "performer"));
        }
        return r;
    }

    private Recording getOrCreateRecording(String isrc, String title, String filePath,
            long durationMs, Artist artist, ScanResult result) throws Exception {
        Dao<Recording, Integer> dao = db().dao(Recording.class);
        List<Recording> existing = dao.queryForEq("isrc", isrc);
        if (!existing.isEmpty()) return existing.get(0);
        Recording rec = new Recording(title);
        rec.setIsrc(isrc);
        rec.setDurationMs(durationMs);
        dao.create(rec);
        if (filePath != null) db().dao(RecordingFile.class).create(new RecordingFile(rec, filePath));
        result.recordingsAdded++;
        db().dao(RecordingCreatorCredit.class).create(new RecordingCreatorCredit(rec, artist, true, "performer"));
        return rec;
    }

    private void getOrCreateTrack(Recording recording, MusicRelease release, int trackNumber,
            long durationMs, ScanResult result) throws Exception {
        Dao<Track, Integer> dao = db().dao(Track.class);
        List<Track> existing = dao.queryBuilder().where()
            .eq("recording_id", recording.getId())
            .and().eq("release_id", release.getId())
            .query();
        if (!existing.isEmpty()) return;
        Track t = new Track();
        t.setRecording(recording);
        t.setRelease(release);
        t.setTrackNumber(trackNumber);
        t.setDurationMs(durationMs);
        dao.create(t);
        result.tracksAdded++;
    }

    private void getOrCreateLocalFile(String isrc, String name, String artistName,
            String releaseName, String filePath, ScanResult result) throws Exception {
        Dao<LocalFile, Integer> dao = db().dao(LocalFile.class);
        if (!dao.queryForEq("isrc", isrc).isEmpty()) return;
        if (!dao.queryForEq("file_path", filePath).isEmpty()) return;
        LocalFile f = new LocalFile(name, filePath);
        f.setArtistName(artistName);
        f.setReleaseName(releaseName);
        f.setIsrc(isrc);
        dao.create(f);
        result.localFilesAdded++;
    }

    // ── Tag helpers ───────────────────────────────────────────────────────────────

    private static String tagOr(Tag tag, FieldKey key, String fallback) {
        if (tag == null) return fallback;
        String v = tag.getFirst(key);
        return (v == null || v.isBlank()) ? fallback : v.trim();
    }

    private static int parseTrackNumber(String raw) {
        if (raw == null || raw.isBlank()) return 0;
        String n = raw.contains("/") ? raw.substring(0, raw.indexOf('/')) : raw;
        try { return Integer.parseInt(n.trim()); } catch (NumberFormatException e) { return 0; }
    }

    private static String stripExtension(String fileName) {
        int dot = fileName.lastIndexOf('.');
        return dot > 0 ? fileName.substring(0, dot) : fileName;
    }
}
