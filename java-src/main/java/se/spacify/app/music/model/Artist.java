package se.spacify.app.music.model;

import com.j256.ormlite.field.DatabaseField;
import com.j256.ormlite.field.ForeignCollectionField;
import com.j256.ormlite.dao.ForeignCollection;
import com.j256.ormlite.table.DatabaseTable;

/** A music {@link Creator} — performer, composer, producer, etc. */
@DatabaseTable(tableName = "artists")
public class Artist extends Creator {

    /** International Standard Name Identifier — nullable until resolved. */
    @DatabaseField(unique = true, canBeNull = true)
    private String isni;

    /** MusicBrainz artist MBID (UUID string). */
    @DatabaseField(unique = true, canBeNull = true)
    private String mbid;

    @ForeignCollectionField(eager = false)
    private ForeignCollection<RecordingCreatorCredit> recordingCredits;

    @ForeignCollectionField(eager = false)
    private ForeignCollection<ReleaseCreatorCredit> releaseCredits;

    public Artist() {}

    public Artist(String name) { setName(name); }

    public String getIsni()         { return isni; }
    public void   setIsni(String v) { this.isni = v; }
    public String getMbid()         { return mbid; }
    public void   setMbid(String v) { this.mbid = v; }
    public ForeignCollection<RecordingCreatorCredit> getRecordingCredits() { return recordingCredits; }
    public ForeignCollection<ReleaseCreatorCredit>   getReleaseCredits()   { return releaseCredits; }

    @Override public String toString() { return getName(); }
}
