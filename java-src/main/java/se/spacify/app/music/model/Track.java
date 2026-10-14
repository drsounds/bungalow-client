package se.spacify.app.music.model;

import com.j256.ormlite.field.DatabaseField;
import com.j256.ormlite.table.DatabaseTable;

/**
 * A {@link Recording}'s concrete appearance on a {@link MusicRelease}: a
 * {@link ContentCollectionRow} carrying position metadata (number, side,
 * duration which may differ from the Recording's).
 */
@DatabaseTable(tableName = "tracks")
public class Track extends ContentCollectionRow<Recording> implements Playable {

    @DatabaseField
    private int trackNumber;

    /** Vinyl side / disc number (e.g. "A", "B", "1", "2"). */
    @DatabaseField(canBeNull = true)
    private String side;

    /** Duration as it appears on the release — may differ from Recording.durationMs. */
    @DatabaseField
    private long durationMs;

    @DatabaseField(foreign = true, foreignAutoRefresh = true, canBeNull = false, columnName = "recording_id")
    private Recording recording;

    @DatabaseField(foreign = true, foreignAutoRefresh = true, canBeNull = false, columnName = "release_id")
    private MusicRelease release;

    public Track() {}

    // ── Playable ──────────────────────────────────────────────────────────────

    @Override public String getPlayUri()    { return recording != null ? recording.getPlayUri() : null; }
    @Override public long   getDurationMs() { return durationMs > 0 ? durationMs : (recording != null ? recording.getDurationMs() : 0); }
    @Override public String getName()      { return recording != null ? recording.getName() : ""; }

    // ── Getters / setters ─────────────────────────────────────────────────────

    public int          getTrackNumber()          { return trackNumber; }
    public void         setTrackNumber(int v)      { this.trackNumber = v; }
    public String       getSide()                  { return side; }
    public void         setSide(String v)          { this.side = v; }
    public void         setDurationMs(long v)      { this.durationMs = v; }
    public Recording    getRecording()             { return recording; }
    public void         setRecording(Recording v)  { this.recording = v; }
    public MusicRelease getRelease()               { return release; }
    public void         setRelease(MusicRelease v) { this.release = v; }

    @Override public String toString() { return trackNumber + ". " + getName(); }
}
