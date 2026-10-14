package se.spacify.app.localmusic.model;

import se.spacify.app.music.model.*;
import com.j256.ormlite.field.DatabaseField;
import com.j256.ormlite.table.DatabaseTable;

/**
 * A self-contained pointer to a playable local audio file.
 * Unlike {@link Recording}/{@link Track}, a LocalFile carries its own
 * denormalised metadata (name, artist, release) so it can be played and
 * displayed without joining the full catalogue model. Used by
 * {@code LocalMusicService} for local playback.
 */
@DatabaseTable(tableName = "local_files")
public class LocalFile implements Playable {

    @DatabaseField(generatedId = true)
    private int id;

    @DatabaseField(canBeNull = false)
    private String name;

    @DatabaseField(canBeNull = true, columnName = "artist_name")
    private String artistName;

    @DatabaseField(canBeNull = true, columnName = "release_name")
    private String releaseName;

    /** International Standard Recording Code, used for cross-Service lookup. */
    @DatabaseField(unique = true, canBeNull = true)
    private String isrc;

    @DatabaseField(canBeNull = false, columnName = "file_path")
    private String filePath;

    public LocalFile() {}

    public LocalFile(String name, String filePath) {
        this.name = name;
        this.filePath = filePath;
    }

    // ── Playable ──────────────────────────────────────────────────────────────

    @Override public String getPlayUri()    { return filePath != null ? "spacify:local:" + filePath : null; }
    @Override public long   getDurationMs() { return 0; }
    @Override public String getName()      { return name; }

    // ── Getters / setters ─────────────────────────────────────────────────────

    public int    getId()                  { return id; }
    public String getTitle()                { return name; }
    public void   setTitle(String v)        { this.name = v; }
    public void   setName(String v)        { this.name = v; }
    public String getArtistName()          { return artistName; }
    public void   setArtistName(String v)  { this.artistName = v; }
    public String getReleaseName()         { return releaseName; }
    public void   setReleaseName(String v) { this.releaseName = v; }
    public String getIsrc()                { return isrc; }
    public void   setIsrc(String v)        { this.isrc = v; }
    public String getFilePath()            { return filePath; }
    public void   setFilePath(String v)    { this.filePath = v; }

    @Override public String toString() { return name; }
}
