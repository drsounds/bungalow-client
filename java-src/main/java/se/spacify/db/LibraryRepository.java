package se.spacify.db;

import com.j256.ormlite.dao.Dao;
import se.spacify.app.music.model.*;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.LinkedHashSet;
import java.util.List;

/**
 * Read/write helpers that compute the joined, display-oriented data the
 * library views need (artist-name strings from the credit join tables,
 * the album a recording appears on, etc.) and that resolve free-text artist
 * names into {@link Artist} rows when persisting credits.
 */
public final class LibraryRepository {

    private LibraryRepository() {}

    private static DatabaseManager db() { return DatabaseManager.getInstance(); }

    /**
     * Canonical ordering of tracks within an album: first by side/disc — nulls
     * first, then natural case-insensitive order so {@code "1" < "2"} and
     * {@code "A" < "B"} — and then by track number ascending. Use this anywhere
     * an album's tracks are presented so they always read in release order.
     */
    public static final Comparator<Track> ALBUM_ORDER =
        Comparator.comparing(Track::getSide,
                             Comparator.nullsFirst(String.CASE_INSENSITIVE_ORDER))
                  .thenComparingInt(Track::getTrackNumber);

    // ── Local file backing (detached from the Recording) ────────────────────────

    /** The local file path backing a recording, or null if none is on record. */
    public static String filePathForRecording(Recording r) {
        if (r == null) return null;
        try {
            List<RecordingFile> files = db().dao(RecordingFile.class).queryForEq("recording_id", r.getId());
            return files.isEmpty() ? null : files.get(0).getFilePath();
        } catch (SQLException e) {
            return null;
        }
    }

    /** Replace the local file backing a recording (clearing it when {@code filePath} is blank). */
    public static void setFilePathForRecording(Recording r, String filePath) throws SQLException {
        if (r == null) return;
        Dao<RecordingFile, Integer> dao = db().dao(RecordingFile.class);
        for (RecordingFile f : dao.queryForEq("recording_id", r.getId())) dao.delete(f);
        if (filePath != null && !filePath.isBlank()) dao.create(new RecordingFile(r, filePath));
    }

    // ── Joined display helpers ──────────────────────────────────────────────────

    /** Comma-joined artist names for a recording, primary credits first. */
    public static String artistNamesForRecording(Recording r) {
        try {
            List<RecordingCreatorCredit> credits =
                db().dao(RecordingCreatorCredit.class).queryForEq("recording_id", r.getId());
            return credits.stream()
                .sorted(Comparator.comparing(RecordingCreatorCredit::isPrimary).reversed())
                .map(c -> c.getArtist() != null ? c.getArtist().getName() : "")
                .filter(n -> !n.isBlank())
                .reduce((a, b) -> a + ", " + b)
                .orElse("");
        } catch (Exception e) {
            return "";
        }
    }

    /** Comma-joined artist names for a release, primary credits first. */
    public static String artistNamesForRelease(MusicRelease r) {
        try {
            List<ReleaseCreatorCredit> credits =
                db().dao(ReleaseCreatorCredit.class).queryForEq("release_id", r.getId());
            return credits.stream()
                .sorted(Comparator.comparing(ReleaseCreatorCredit::isPrimary).reversed())
                .map(c -> c.getArtist() != null ? c.getArtist().getName() : "")
                .filter(n -> !n.isBlank())
                .reduce((a, b) -> a + ", " + b)
                .orElse("");
        } catch (Exception e) {
            return "";
        }
    }

    /** The first primary artist name for a recording, or "" if none. */
    public static String primaryArtistForRecording(Recording r) {
        try {
            return db().dao(RecordingCreatorCredit.class).queryForEq("recording_id", r.getId()).stream()
                .filter(RecordingCreatorCredit::isPrimary)
                .map(c -> c.getArtist() != null ? c.getArtist().getName() : "")
                .filter(n -> !n.isBlank())
                .findFirst()
                .orElse("");
        } catch (Exception e) {
            return "";
        }
    }

    /** Comma-joined titles of releases this recording appears on (via tracks). */
    public static String albumForRecording(Recording r) {
        try {
            List<Track> tracks = db().dao(Track.class).queryForEq("recording_id", r.getId());
            LinkedHashSet<String> titles = new LinkedHashSet<>();
            for (Track t : tracks) {
                if (t.getRelease() != null && t.getRelease().getName() != null) {
                    titles.add(t.getRelease().getName());
                }
            }
            return String.join(", ", titles);
        } catch (Exception e) {
            return "";
        }
    }

    // ── Artist resolution ───────────────────────────────────────────────────────

    /** Find an artist by exact name, creating one if none exists. */
    public static Artist findOrCreateArtist(String name) throws SQLException {
        String trimmed = name.trim();
        List<Artist> existing = db().dao(Artist.class).queryForEq("name", trimmed);
        if (!existing.isEmpty()) return existing.get(0);
        Artist a = new Artist(trimmed);
        db().dao(Artist.class).create(a);
        return a;
    }

    /** Parse a comma-separated artist list into trimmed, non-blank names. */
    public static List<String> parseArtistNames(String csv) {
        List<String> out = new ArrayList<>();
        if (csv == null) return out;
        for (String part : csv.split(",")) {
            String n = part.trim();
            if (!n.isEmpty()) out.add(n);
        }
        return out;
    }

    /** Replace a recording's primary artist credits with the given names. */
    public static void setRecordingArtists(Recording r, List<String> names) throws SQLException {
        Dao<RecordingCreatorCredit, Integer> dao = db().dao(RecordingCreatorCredit.class);
        dao.delete(dao.queryForEq("recording_id", r.getId()));
        for (String n : names) {
            Artist a = findOrCreateArtist(n);
            dao.create(new RecordingCreatorCredit(r, a, true, "performer"));
        }
    }

    /** Replace a release's primary artist credits with the given names. */
    public static void setReleaseArtists(MusicRelease r, List<String> names) throws SQLException {
        Dao<ReleaseCreatorCredit, Integer> dao = db().dao(ReleaseCreatorCredit.class);
        dao.delete(dao.queryForEq("release_id", r.getId()));
        for (String n : names) {
            Artist a = findOrCreateArtist(n);
            dao.create(new ReleaseCreatorCredit(r, a, true, "performer"));
        }
    }
}
