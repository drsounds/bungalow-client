package se.spacify.app.playlist.model;

import se.spacify.app.music.model.*;
import com.j256.ormlite.field.DatabaseField;
import com.j256.ormlite.table.DatabaseTable;

/**
 * One positioned entry in a {@link Playlist}. The content is referenced by a
 * {@code spacify:} URI rather than a hard foreign key, so a single playlist can
 * mix content of different types (recordings, and later video/game items) and
 * resolve each via the identifier/name lookup model.
 */
@DatabaseTable(tableName = "playlist_rows")
public class PlaylistRow extends ContentCollectionRow<Content<?>> {

    @DatabaseField(foreign = true, foreignAutoRefresh = true, canBeNull = false, columnName = "playlist_id")
    private Playlist playlist;

    /** Denormalised playlist name, handy for flat listings. */
    @DatabaseField(canBeNull = true, columnName = "playlist_name")
    private String playlistName;

    /** spacify: URI of the referenced content. */
    @DatabaseField(canBeNull = false, columnName = "content_uri")
    private String contentUri;

    /**
     * The kind of playable this row references (see {@link se.spacify.app.music.model.PlayableKind}),
     * stored as its lower-case id. Nullable for rows written before the column
     * existed — those recover the kind from {@link #contentUri}'s scheme.
     */
    @DatabaseField(canBeNull = true)
    private String kind;

    // Denormalised display metadata so a row rebuilds into a Playable without a
    // cross-service lookup (the referenced content may live on a remote service).
    @DatabaseField(canBeNull = true)
    private String title;

    @DatabaseField(canBeNull = true)
    private String artist;

    @DatabaseField(columnName = "duration_ms")
    private long durationMs;

    public PlaylistRow() {}

    public PlaylistRow(Playlist playlist, int position, String contentUri) {
        this.playlist   = playlist;
        this.contentUri = contentUri;
        this.playlistName = playlist != null ? playlist.getName() : null;
        setPosition(position);
    }

    public Playlist getPlaylist()           { return playlist; }
    public void     setPlaylist(Playlist v) { this.playlist = v; }
    public String   getPlaylistName()       { return playlistName; }
    public void     setPlaylistName(String v) { this.playlistName = v; }
    public String   getContentUri()         { return contentUri; }
    public void     setContentUri(String v) { this.contentUri = v; }
    public String   getKind()               { return kind; }
    public void     setKind(String v)       { this.kind = v; }
    public String   getName()              { return title; }
    public void     setName(String v)      { this.title = v; }
    public String   getArtist()             { return artist; }
    public void     setArtist(String v)     { this.artist = v; }
    public long     getDurationMs()         { return durationMs; }
    public void     setDurationMs(long v)   { this.durationMs = v; }
}
