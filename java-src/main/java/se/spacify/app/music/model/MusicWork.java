package se.spacify.app.music.model;

import com.j256.ormlite.field.DatabaseField;
import com.j256.ormlite.field.ForeignCollectionField;
import com.j256.ormlite.dao.ForeignCollection;
import com.j256.ormlite.table.DatabaseTable;

/**
 * An abstract musical work (composition / song).
 * One MusicWork may have many Recordings (covers, live versions, remasters).
 */
@DatabaseTable(tableName = "music_works")
public class MusicWork extends Node {

    @DatabaseField(generatedId = true)
    private int id;

    /** International Standard Musical Work Code — nullable until catalogued. */
    @DatabaseField(unique = true, canBeNull = true)
    private String iswc;

    @ForeignCollectionField(eager = false)
    private ForeignCollection<Recording> recordings;

    public MusicWork() {}

    public MusicWork(String name) { setName(name); }

    public int    getId()                            { return id; }
    public String getIswc()                          { return iswc; }
    public void   setIswc(String iswc)               { this.iswc = iswc; }
    public ForeignCollection<Recording> getRecordings() { return recordings; }

    @Override public String toString() { return getName(); }
}
