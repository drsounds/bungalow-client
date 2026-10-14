package se.spacify.app.playlist.model;

import se.spacify.app.music.model.*;
import com.j256.ormlite.dao.ForeignCollection;
import com.j256.ormlite.field.DatabaseField;
import com.j256.ormlite.field.ForeignCollectionField;
import com.j256.ormlite.table.DatabaseTable;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

/**
 * A named, ordered collection of mixed {@link Content} — the persistence behind a
 * user playlist. Being a {@link ContentCollection}, its entries ({@link PlaylistRow})
 * reference content by {@code spacify:} URI, so recordings, future video/game
 * items, etc. can be freely mixed in one playlist.
 *
 * <p>The resolved {@link Playable} items are held transiently in {@link #getItems()}
 * (populated by the playlist service on load); the durable rows live in
 * {@link #getRows()}.
 */
@DatabaseTable(tableName = "playlists")
public class Playlist extends ContentCollection<Content<?>> {

    /** Stable, shareable public identifier behind the {@code spacify:playlist:<uuid>} URI. */
    @DatabaseField(unique = true, canBeNull = false)
    private String uuid = UUID.randomUUID().toString();

    @ForeignCollectionField(eager = false)
    private ForeignCollection<PlaylistRow> rows;

    /** Runtime, resolved items — not persisted directly (the rows are). */
    private final List<Playable> items = new ArrayList<>();

    public Playlist() {}

    public Playlist(String name) { setName(name); }

    /** The stable public id used by {@link se.spacify.app.playlist.service.PlaylistService}. */
    public String getPublicId()       { return uuid; }
    public void   setPublicId(String v) { this.uuid = v; }

    public ForeignCollection<PlaylistRow> getRows() { return rows; }

    /** The resolved, ordered playable items for this playlist. */
    public List<Playable> getItems() { return items; }

    @Override public String toString() { return getName(); }
}
