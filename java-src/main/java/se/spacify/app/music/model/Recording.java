package se.spacify.app.music.model;

import com.j256.ormlite.dao.ForeignCollection;
import com.j256.ormlite.field.DatabaseField;
import com.j256.ormlite.field.ForeignCollectionField;
import com.j256.ormlite.table.DatabaseTable;

/**
 * A specific audio recording of a {@link MusicWork}, identified globally by ISRC.
 *
 * <p>Local playback is no longer modelled on the recording itself: file paths
 * live in their own {@link RecordingFile} table so that a local file is just one
 * more way to play a recording, on equal footing with any streaming service.
 */
@DatabaseTable(tableName = "recordings")
public class Recording extends Content<RecordingCreatorCredit> implements Playable {

    /** International Standard Recording Code. */
    @DatabaseField(unique = true, canBeNull = true)
    private String isrc;

    @DatabaseField
    private long durationMs;

    @DatabaseField(foreign = true, foreignAutoRefresh = true, canBeNull = true, columnName = "music_work_id")
    private MusicWork musicWork;

    @ForeignCollectionField(eager = false)
    private ForeignCollection<RecordingCreatorCredit> creatorCredits;

    public Recording() {}

    public Recording(String name) { setName(name); }

    // ── Content ───────────────────────────────────────────────────────────────

    @Override public ForeignCollection<RecordingCreatorCredit> getCreatorCredits() { return creatorCredits; }
    @Override public void setCreatorCredits(ForeignCollection<RecordingCreatorCredit> v) { this.creatorCredits = v; }

    // ── Playable ──────────────────────────────────────────────────────────────

    @Override
    public String getPlayUri() {
        // The canonical, portable content identifier for a recording (RFC-0002).
        return isrc != null ? se.spacify.app.music.net.MusikUri.isrc(isrc).toString() : null;
    }

    @Override public long getDurationMs() { return durationMs; }
    // getName() is provided by Node and satisfies Playable.

    // ── Getters / setters ─────────────────────────────────────────────────────

    public String    getIsrc()                 { return isrc; }
    public void      setIsrc(String v)         { this.isrc = v; }
    public void      setDurationMs(long v)     { this.durationMs = v; }
    public MusicWork getMusicWork()            { return musicWork; }
    public void      setMusicWork(MusicWork v) { this.musicWork = v; }

    @Override public String toString() { return getName(); }
}
