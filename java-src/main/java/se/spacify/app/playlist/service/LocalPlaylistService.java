package se.spacify.app.playlist.service;

import com.j256.ormlite.dao.Dao;
import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;
import se.spacify.db.DatabaseManager;
import se.spacify.db.LibraryRepository;
import se.spacify.app.music.model.Playable;
import se.spacify.app.music.model.PlayableKind;
import se.spacify.app.music.model.PlayableRef;
import se.spacify.app.playlist.model.Playlist;
import se.spacify.app.playlist.model.PlaylistRow;
import se.spacify.app.music.model.Recording;
import se.spacify.app.music.model.Track;
import se.spacify.app.playlist.PlaylistEvents;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;

/**
 * Database-backed {@link PlaylistService}: the local store behind the user's own
 * playlists. Playlists and their ordered rows persist via ORMLite; each row keeps
 * a {@code spacify:} URI plus denormalised display metadata so a playlist renders
 * (and its items play) without any cross-service lookup. Mutations fire
 * {@link PlaylistEvents} so mirroring views refresh.
 */
public class LocalPlaylistService implements PlaylistService {

    // ── Service identity ──────────────────────────────────────────────────────

    @Override public String getId()   { return "spacify.local.playlists"; }
    @Override public String getName() { return "Your Playlists"; }

    @Override public void onRegister(AspectManager<? extends Aspect> aspectManager) {}

    // ── Reads ─────────────────────────────────────────────────────────────────

    @Override
    public List<Playlist> getPlaylists() {
        List<Playlist> out = new ArrayList<>();
        try {
            for (Playlist pl : playlistDao().queryForAll()) {
                loadItems(pl);
                out.add(pl);
            }
        } catch (SQLException e) {
            throw new RuntimeException("Failed to list playlists", e);
        }
        return out;
    }

    @Override
    public Playlist getPlaylist(String id) {
        Playlist pl = find(id);
        if (pl != null) loadItems(pl);
        return pl;
    }

    // ── Mutations ─────────────────────────────────────────────────────────────

    @Override public boolean isEditable() { return true; }

    @Override
    public Playlist createPlaylist(String name) {
        try {
            Playlist pl = new Playlist(name);
            playlistDao().create(pl);
            PlaylistEvents.fireChanged();
            return pl;
        } catch (SQLException e) {
            throw new RuntimeException("Failed to create playlist", e);
        }
    }

    @Override
    public void renamePlaylist(String id, String name) {
        Playlist pl = require(id);
        try {
            pl.setName(name);
            playlistDao().update(pl);
            PlaylistEvents.fireChanged();
        } catch (SQLException e) {
            throw new RuntimeException("Failed to rename playlist", e);
        }
    }

    @Override
    public void deletePlaylist(String id) {
        Playlist pl = find(id);
        if (pl == null) return;
        try {
            Dao<PlaylistRow, Integer> rowDao = rowDao();
            for (PlaylistRow r : rowsOf(pl)) rowDao.delete(r);
            playlistDao().delete(pl);
            PlaylistEvents.fireChanged();
        } catch (SQLException e) {
            throw new RuntimeException("Failed to delete playlist", e);
        }
    }

    @Override
    public void addToPlaylist(String playlistId, Playable item, String kind) {
        Playlist pl = require(playlistId);
        if (item == null) return;
        try {
            PlaylistRow row = new PlaylistRow(pl, rowsOf(pl).size(), item.getPlayUri());
            row.setName(item.getName());
            row.setDurationMs(item.getDurationMs());
            row.setArtist(artistOf(item));
            row.setKind(kindOf(item, kind));
            rowDao().create(row);
            PlaylistEvents.fireChanged();
        } catch (SQLException e) {
            throw new RuntimeException("Failed to add to playlist", e);
        }
    }

    /** The stored kind id: the caller's when given, else derived from the play URI. */
    private static String kindOf(Playable item, String kind) {
        if (kind != null && !kind.isBlank()) return kind;
        if (item instanceof PlayableRef ref && ref.getKind() != null) return ref.getKind().id();
        PlayableKind derived = PlayableKind.fromUri(item.getPlayUri());
        return derived != null ? derived.id() : null;
    }

    @Override
    public void removeFromPlaylist(String playlistId, int index) {
        Playlist pl = require(playlistId);
        try {
            List<PlaylistRow> rows = rowsOf(pl);
            if (index < 0 || index >= rows.size()) return;
            Dao<PlaylistRow, Integer> rowDao = rowDao();
            rowDao.delete(rows.get(index));
            // Re-pack positions so they stay contiguous and 0-based.
            for (int i = index + 1; i < rows.size(); i++) {
                PlaylistRow r = rows.get(i);
                r.setPosition(i - 1);
                rowDao.update(r);
            }
            PlaylistEvents.fireChanged();
        } catch (SQLException e) {
            throw new RuntimeException("Failed to remove from playlist", e);
        }
    }

    @Override
    public void moveRow(String playlistId, int from, int to) {
        Playlist pl = require(playlistId);
        try {
            List<PlaylistRow> rows = rowsOf(pl);
            if (from < 0 || from >= rows.size() || to < 0 || to >= rows.size() || from == to) return;
            PlaylistRow moved = rows.remove(from);
            rows.add(to, moved);
            Dao<PlaylistRow, Integer> rowDao = rowDao();
            for (int i = 0; i < rows.size(); i++) {
                PlaylistRow r = rows.get(i);
                if (r.getPosition() != i) { r.setPosition(i); rowDao.update(r); }
            }
            PlaylistEvents.fireChanged();
        } catch (SQLException e) {
            throw new RuntimeException("Failed to reorder playlist", e);
        }
    }

    // ── Internals ─────────────────────────────────────────────────────────────

    /** Best-effort artist string for the denormalised row metadata. */
    private static String artistOf(Playable item) {
        if (item instanceof Recording rec) return LibraryRepository.artistNamesForRecording(rec);
        if (item instanceof Track t && t.getRecording() != null)
            return LibraryRepository.artistNamesForRecording(t.getRecording());
        if (item instanceof PlaylistItem pi) return pi.getArtist();
        return null;
    }

    private Playlist find(String id) {
        if (id == null) return null;
        try {
            return playlistDao().queryForEq("uuid", id).stream().findFirst().orElse(null);
        } catch (SQLException e) {
            throw new RuntimeException("Failed to look up playlist " + id, e);
        }
    }

    private Playlist require(String id) {
        Playlist pl = find(id);
        if (pl == null) throw new IllegalArgumentException("No such playlist: " + id);
        return pl;
    }

    /** Rows of a playlist in stored order. */
    private List<PlaylistRow> rowsOf(Playlist pl) throws SQLException {
        List<PlaylistRow> rows = rowDao().queryForEq("playlist_id", pl.getId());
        rows.sort((a, b) -> Integer.compare(a.getPosition(), b.getPosition()));
        return rows;
    }

    /** Populate a playlist's transient {@link Playlist#getItems()} from its rows. */
    private void loadItems(Playlist pl) {
        try {
            pl.getItems().clear();
            for (PlaylistRow r : rowsOf(pl))
                pl.getItems().add(new PlaylistItem(r.getContentUri(), r.getName(), r.getArtist(), r.getDurationMs()));
        } catch (SQLException e) {
            throw new RuntimeException("Failed to load playlist items", e);
        }
    }

    private static Dao<Playlist, Integer>    playlistDao() { return DatabaseManager.getInstance().dao(Playlist.class); }
    private static Dao<PlaylistRow, Integer> rowDao()      { return DatabaseManager.getInstance().dao(PlaylistRow.class); }
}
