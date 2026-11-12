package se.bungalow.library;

import com.j256.ormlite.dao.Dao;
import org.jaudiotagger.audio.AudioFile;
import org.jaudiotagger.audio.AudioFileIO;
import org.jaudiotagger.tag.FieldKey;
import org.jaudiotagger.tag.Tag;
import se.bungalow.db.DatabaseManager;
import se.bungalow.db.LibraryRepository;
import se.bungalow.app.music.model.*;
import se.bungalow.app.localmusic.model.LocalFile;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Recursively scans a folder for MP3 files and imports them into the library.
 * For each file it reads ID3 metadata via jaudiotagger and upserts a
 * {@link LocalFile} plus the generic catalogue graph
 * (Artist → Release → Recording → Track): every credited artist (split on
 * ";" and feat./ft., including a "(feat. X)" title suffix), the release with
 * its album artist and year, the recording with its ISRC and file, and the
 * track with its number (and disc, for multi-disc albums). Rows are
 * de-duplicated by name/ISRC so re-scanning the same folder does not create
 * duplicates.
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

    /** The embedded tags of one file, normalised the way the C++ AudioImporter does. */
    private record FileTags(String title, List<String> artists, String albumArtist, String album,
                            String releaseDate, String isrc, int trackNumber, String side,
                            long durationMs) {}

    private Imported importFile(File file, ScanResult result) throws Exception {
        FileTags t = readTags(file);
        String path = file.getAbsolutePath();

        List<Artist> artists = new ArrayList<>();
        for (String name : t.artists()) artists.add(getOrCreateArtist(name, result));

        MusicRelease release   = getOrCreateRelease(t.album(), t.albumArtist(), t.releaseDate(), result);
        Recording    recording = getOrCreateRecording(t.isrc(), t.title(), t.durationMs(), artists, result);
        addRecordingFile(recording, path);
        getOrCreateTrack(recording, release, t.trackNumber(), t.side(), t.durationMs(), result);

        String artistNames = String.join(", ", t.artists());
        getOrCreateLocalFile(t.isrc(), t.title(), artistNames, t.album(), path, result);
        return new Imported(t.title(), artistNames, t.album());
    }

    private static FileTags readTags(File file) throws Exception {
        AudioFile audio = AudioFileIO.read(file);
        Tag tag = audio.getTag();

        String title = tagOr(tag, FieldKey.TITLE, null);
        List<String> artists = new ArrayList<>();
        if (tag != null)
            for (String value : tag.getAll(FieldKey.ARTIST)) addArtists(artists, value);

        // Untagged file: fall back to "Artist - Title" in the filename.
        String stem = stripExtension(file.getName());
        int sep = stem.indexOf(" - ");
        if (artists.isEmpty() && sep > 0) addArtists(artists, stem.substring(0, sep));
        if (title == null) title = sep > 0 ? stem.substring(sep + 3).trim() : stem;

        // "Song (feat. X)" credits X and leaves "Song" as the title.
        Matcher feat = FEATURING_IN_TITLE.matcher(title);
        if (feat.find()) {
            addArtists(artists, feat.group(2));
            title = title.substring(0, feat.start()).trim();
        }
        if (artists.isEmpty()) artists.add("Unknown Artist");

        String albumArtist = tagOr(tag, FieldKey.ALBUM_ARTIST, artists.get(0));
        String album       = tagOr(tag, FieldKey.ALBUM, "Unknown Album");
        String year        = tagOr(tag, FieldKey.YEAR, null);
        int    trackNumber = parseNumber(tag != null ? tag.getFirst(FieldKey.TRACK) : null);

        // Only multi-disc albums get a side, so single-disc track order is unchanged.
        int disc      = parseNumber(tag != null ? tag.getFirst(FieldKey.DISC_NO) : null);
        int discTotal = parseNumber(tag != null ? tag.getFirst(FieldKey.DISC_TOTAL) : null);
        String side   = (disc > 1 || discTotal > 1) ? String.valueOf(disc) : null;

        long durationMs = audio.getAudioHeader() != null
                ? audio.getAudioHeader().getTrackLength() * 1000L : 0L;

        String rawIsrc = tagOr(tag, FieldKey.ISRC, null);
        String isrc = rawIsrc != null ? rawIsrc : "SPLOCAL-" + Math.abs(file.getAbsolutePath().hashCode());

        return new FileTags(title, artists, albumArtist, album, year, isrc, trackNumber, side, durationMs);
    }

    // ── Upsert helpers (count only newly created rows) ─────────────────────────────

    /** Find an artist by name (case-insensitive), creating one if none exists. */
    private Artist getOrCreateArtist(String name, ScanResult result) throws Exception {
        Dao<Artist, Integer> dao = db().dao(Artist.class);
        // SQLite LIKE is case-insensitive; '%'/'_' in a name only widen the match,
        // so confirm each candidate exactly.
        for (Artist a : dao.queryBuilder().where().like("name", name).query())
            if (name.equalsIgnoreCase(a.getName())) return a;
        Artist a = new Artist(name);
        dao.create(a);
        result.artistsAdded++;
        return a;
    }

    private MusicRelease getOrCreateRelease(String name, String albumArtist, String releaseDate,
            ScanResult result) throws Exception {
        Dao<MusicRelease, Integer> dao = db().dao(MusicRelease.class);
        for (MusicRelease r : dao.queryForEq("name", name)) {
            // Same name AND same album artist counts as the same release, so we
            // don't merge unrelated albums that happen to share a name.
            if (albumArtist == null || albumArtist.isBlank()
                    || LibraryRepository.artistNamesForRelease(r).equalsIgnoreCase(albumArtist.trim())) {
                if (r.getReleaseDate() == null && releaseDate != null) {
                    r.setReleaseDate(releaseDate);
                    dao.update(r);
                }
                return r;
            }
        }
        MusicRelease r = new MusicRelease(name);
        r.setType(MusicRelease.ReleaseType.ALBUM);
        r.setReleaseDate(releaseDate);
        dao.create(r);
        result.releasesAdded++;
        if (albumArtist != null && !albumArtist.isBlank()) {
            Artist a = getOrCreateArtist(albumArtist, result);
            db().dao(ReleaseCreatorCredit.class).create(new ReleaseCreatorCredit(r, a, true, "performer"));
        }
        return r;
    }

    /** A recording by ISRC; a new one credits every artist, the first as primary. */
    private Recording getOrCreateRecording(String isrc, String name, long durationMs,
            List<Artist> artists, ScanResult result) throws Exception {
        Dao<Recording, Integer> dao = db().dao(Recording.class);
        List<Recording> existing = dao.queryForEq("isrc", isrc);
        if (!existing.isEmpty()) return existing.get(0);
        Recording rec = new Recording(name);
        rec.setIsrc(isrc);
        rec.setDurationMs(durationMs);
        dao.create(rec);
        result.recordingsAdded++;
        Dao<RecordingCreatorCredit, Integer> credits = db().dao(RecordingCreatorCredit.class);
        for (int i = 0; i < artists.size(); i++)
            credits.create(new RecordingCreatorCredit(rec, artists.get(i), i == 0, i == 0 ? "performer" : "featured"));
        return rec;
    }

    /** Back the recording with this file, unless it already is (re-scans, other copies). */
    private void addRecordingFile(Recording recording, String path) throws Exception {
        Dao<RecordingFile, Integer> dao = db().dao(RecordingFile.class);
        boolean known = dao.queryBuilder().where()
            .eq("recording_id", recording.getId())
            .and().eq("file_path", path)
            .countOf() > 0;
        if (!known) dao.create(new RecordingFile(recording, path));
    }

    private void getOrCreateTrack(Recording recording, MusicRelease release, int trackNumber,
            String side, long durationMs, ScanResult result) throws Exception {
        Dao<Track, Integer> dao = db().dao(Track.class);
        List<Track> existing = dao.queryBuilder().where()
            .eq("recording_id", recording.getId())
            .and().eq("release_id", release.getId())
            .query();
        if (!existing.isEmpty()) {
            // Fill in a position the track was created without.
            Track t = existing.get(0);
            if (t.getTrackNumber() == 0 && trackNumber > 0) {
                t.setTrackNumber(trackNumber);
                if (t.getSide() == null) t.setSide(side);
                dao.update(t);
            }
            return;
        }
        Track t = new Track();
        t.setRecording(recording);
        t.setRelease(release);
        t.setTrackNumber(trackNumber);
        t.setSide(side);
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

    /** "Song (feat. X)" / "Song [ft. X & Y]" at the end of a title. */
    private static final Pattern FEATURING_IN_TITLE =
        Pattern.compile("\\s*[(\\[](feat\\.?|ft\\.?|featuring)\\s+([^)\\]]+)[)\\]]\\s*$", Pattern.CASE_INSENSITIVE);

    /** Separators between credited artists; commas and "&" stay, as they're often part of a name. */
    private static final Pattern ARTIST_SEPARATOR =
        Pattern.compile("\\s*;\\s*|\\s+(?:featuring|feat\\.?|ft\\.?)\\s+", Pattern.CASE_INSENSITIVE);

    /** Split an artist tag value and append the names not already in {@code out}. */
    private static void addArtists(List<String> out, String value) {
        if (value == null) return;
        for (String part : ARTIST_SEPARATOR.split(value)) {
            String name = part.trim();
            if (!name.isEmpty() && out.stream().noneMatch(name::equalsIgnoreCase)) out.add(name);
        }
    }

    private static String tagOr(Tag tag, FieldKey key, String fallback) {
        if (tag == null) return fallback;
        String v = tag.getFirst(key);
        return (v == null || v.isBlank()) ? fallback : v.trim();
    }

    /** The leading number of "3" or "3/12"; 0 when absent or unparsable. */
    private static int parseNumber(String raw) {
        if (raw == null || raw.isBlank()) return 0;
        String n = raw.contains("/") ? raw.substring(0, raw.indexOf('/')) : raw;
        try { return Integer.parseInt(n.trim()); } catch (NumberFormatException e) { return 0; }
    }

    private static String stripExtension(String fileName) {
        int dot = fileName.lastIndexOf('.');
        return dot > 0 ? fileName.substring(0, dot) : fileName;
    }
}
