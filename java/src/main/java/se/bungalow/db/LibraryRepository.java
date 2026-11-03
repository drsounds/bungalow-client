package se.bungalow.db;

import com.j256.ormlite.dao.Dao;
import com.j256.ormlite.stmt.DeleteBuilder;
import com.j256.ormlite.stmt.UpdateBuilder;
import se.bungalow.app.media.model.MusicServiceTrack;
import se.bungalow.app.music.model.*;

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

    /** The first primary artist credited on a recording, or null if none. */
    public static Artist primaryArtistEntityForRecording(Recording r) {
        try {
            return db().dao(RecordingCreatorCredit.class).queryForEq("recording_id", r.getId()).stream()
                .filter(RecordingCreatorCredit::isPrimary)
                .map(RecordingCreatorCredit::getArtist)
                .filter(a -> a != null && a.getName() != null)
                .findFirst()
                .orElse(null);
        } catch (Exception e) {
            return null;
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

    // ── Browsing ────────────────────────────────────────────────────────────────

    /** A release's tracks in {@link #ALBUM_ORDER}. */
    public static List<Track> tracksForRelease(MusicRelease r) throws SQLException {
        List<Track> tracks = new ArrayList<>(db().dao(Track.class).queryForEq("release_id", r.getId()));
        tracks.sort(ALBUM_ORDER);
        return tracks;
    }

    /** Every appearance (track) of a recording, one per release it is on. */
    public static List<Track> tracksForRecording(Recording r) throws SQLException {
        return db().dao(Track.class).queryForEq("recording_id", r.getId());
    }

    /** Recordings an artist is credited on, by name. */
    public static List<Recording> recordingsForArtist(Artist a) throws SQLException {
        List<Recording> out = new ArrayList<>();
        for (RecordingCreatorCredit c : db().dao(RecordingCreatorCredit.class).queryForEq("artist_id", a.getId())) {
            if (c.getRecording() != null && out.stream().noneMatch(r -> r.getId() == c.getRecording().getId()))
                out.add(c.getRecording());
        }
        out.sort(Comparator.comparing(Recording::getName, Comparator.nullsLast(String.CASE_INSENSITIVE_ORDER)));
        return out;
    }

    /** The next free track number on a release (one past the highest). */
    public static int nextTrackNumber(MusicRelease r) throws SQLException {
        int max = 0;
        for (Track t : db().dao(Track.class).queryForEq("release_id", r.getId()))
            max = Math.max(max, t.getTrackNumber());
        return max + 1;
    }

    // ── Cascading deletes ───────────────────────────────────────────────────────
    //
    // ORMLite doesn't enforce foreign keys on SQLite, so deleting a row would
    // leave credits/tracks pointing at nothing (they then render as blank rows).
    // These remove an entity together with the rows that only make sense with it.

    /** Delete an artist and their credits; the credited recordings and releases stay. */
    public static void deleteArtist(Artist a) throws SQLException {
        deleteWhere(RecordingCreatorCredit.class, "artist_id", a.getId());
        deleteWhere(ReleaseCreatorCredit.class, "artist_id", a.getId());
        db().dao(Artist.class).delete(a);
    }

    /** Delete a recording with its credits, local files and tracks. */
    public static void deleteRecording(Recording r) throws SQLException {
        for (Track t : tracksForRecording(r)) deleteTrack(t);
        deleteWhere(RecordingCreatorCredit.class, "recording_id", r.getId());
        deleteWhere(RecordingFile.class, "recording_id", r.getId());
        db().dao(Recording.class).delete(r);
    }

    /** Delete a release with its credits and tracks; the recordings stay. */
    public static void deleteRelease(MusicRelease r) throws SQLException {
        for (Track t : db().dao(Track.class).queryForEq("release_id", r.getId())) deleteTrack(t);
        deleteWhere(ReleaseCreatorCredit.class, "release_id", r.getId());
        db().dao(MusicRelease.class).delete(r);
    }

    /**
     * Delete a track. A remembered "Play with…" pick bound to it is unbound
     * rather than deleted: it still matches the track by ISRC/URI.
     */
    public static void deleteTrack(Track t) throws SQLException {
        UpdateBuilder<MusicServiceTrack, Integer> unbind = db().dao(MusicServiceTrack.class).updateBuilder();
        unbind.updateColumnValue("track_id", null).where().eq("track_id", t.getId());
        unbind.update();
        db().dao(Track.class).delete(t);
    }

    private static <T> void deleteWhere(Class<T> entity, String column, int id) throws SQLException {
        DeleteBuilder<T, Integer> delete = db().dao(entity).deleteBuilder();
        delete.where().eq(column, id);
        delete.delete();
    }
}
