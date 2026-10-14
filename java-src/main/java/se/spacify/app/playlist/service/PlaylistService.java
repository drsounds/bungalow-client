package se.spacify.app.playlist.service;

import se.spacify.app.music.model.Playable;
import se.spacify.app.playlist.model.Playlist;
import se.spacify.service.Service;

import java.util.List;

/**
 * Playlist aspect: listing and (optionally) editing playlists. Implemented by
 * Services that own playlists — a local store, or a remote account exposing its
 * own and curated playlists — and discovered via
 * {@link se.spacify.service.ServiceManager#getServices(Class)}.
 *
 * <p>Read-only providers implement just {@link #getPlaylists()} and
 * {@link #getPlaylist(String)}; editable ones override {@link #isEditable()} and
 * the mutators. The {@link Playable} items come from the same domain model the
 * media/library aspects use, so a playlist can be handed straight to the play
 * queue.
 */
public interface PlaylistService extends Service {

    List<Playlist> getPlaylists();

    /** The playlist with the given id, or null if absent. */
    Playlist getPlaylist(String id);

    /** Whether this Service supports creating and modifying playlists. */
    default boolean isEditable() { return false; }

    default Playlist createPlaylist(String name) {
        throw new UnsupportedOperationException("playlist Service is read-only");
    }

    default void renamePlaylist(String id, String name) {
        throw new UnsupportedOperationException("playlist Service is read-only");
    }

    default void deletePlaylist(String id) {
        throw new UnsupportedOperationException("playlist Service is read-only");
    }

    default void addToPlaylist(String playlistId, Playable item) {
        addToPlaylist(playlistId, item, null);
    }

    /**
     * Add {@code item}, recording {@code kind} (a {@link se.spacify.app.music.model.PlayableKind}
     * id) so the row remembers what it references. A {@code null} kind is derived
     * from the item's play URI. Read-only providers throw.
     */
    default void addToPlaylist(String playlistId, Playable item, String kind) {
        throw new UnsupportedOperationException("playlist Service is read-only");
    }

    /** Remove the item at {@code index} from the playlist. */
    default void removeFromPlaylist(String playlistId, int index) {
        throw new UnsupportedOperationException("playlist Service is read-only");
    }

    /** Move the item at {@code from} to {@code to}, re-packing positions. */
    default void moveRow(String playlistId, int from, int to) {
        throw new UnsupportedOperationException("playlist Service is read-only");
    }
}
