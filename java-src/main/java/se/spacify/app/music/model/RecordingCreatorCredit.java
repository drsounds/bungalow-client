package se.spacify.app.music.model;

import com.j256.ormlite.field.DatabaseField;
import com.j256.ormlite.table.DatabaseTable;

/** Credits an {@link Artist} on a {@link Recording}, with role metadata. */
@DatabaseTable(tableName = "recording_artist_credits")
public class RecordingCreatorCredit extends CreatorCredit<Recording> {

    @DatabaseField(foreign = true, foreignAutoRefresh = true, canBeNull = false, columnName = "recording_id")
    private Recording recording;

    @DatabaseField(foreign = true, foreignAutoRefresh = true, canBeNull = false, columnName = "artist_id")
    private Artist artist;

    public RecordingCreatorCredit() {}

    public RecordingCreatorCredit(Recording recording, Artist artist, boolean primary, String role) {
        this.recording = recording;
        this.artist    = artist;
        setPrimary(primary);
        setRole(role);
    }

    public Recording getRecording()            { return recording; }
    public void      setRecording(Recording v) { this.recording = v; }
    public Artist    getArtist()               { return artist; }
    public void      setArtist(Artist v)       { this.artist = v; }

    @Override public Creator   getCreator() { return artist; }
    @Override public Recording getContent() { return recording; }
}
