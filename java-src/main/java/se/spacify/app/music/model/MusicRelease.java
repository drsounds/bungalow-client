package se.spacify.app.music.model;

import com.j256.ormlite.dao.ForeignCollection;
import com.j256.ormlite.field.DataType;
import com.j256.ormlite.field.DatabaseField;
import com.j256.ormlite.field.ForeignCollectionField;
import com.j256.ormlite.table.DatabaseTable;

/** A music release (album, single, EP…) composed of {@link Recording}s. */
@DatabaseTable(tableName = "releases")
public class MusicRelease extends Release<Recording> {

    public enum ReleaseType { ALBUM, SINGLE, EP, COMPILATION, BROADCAST, OTHER }

    /** MusicBrainz Release MBID (UUID string). */
    @DatabaseField(unique = true, canBeNull = true)
    private String mbid;

    /** UPC or EAN barcode. */
    @DatabaseField(unique = true, canBeNull = true)
    private String upc;

    /** ISO 8601 date string (YYYY-MM-DD or YYYY-MM or YYYY). */
    @DatabaseField(canBeNull = true)
    private String releaseDate;

    @DatabaseField(dataType = DataType.ENUM_STRING, canBeNull = true)
    private ReleaseType type;

    @ForeignCollectionField(eager = false)
    private ForeignCollection<Track> tracks;

    @ForeignCollectionField(eager = false)
    private ForeignCollection<ReleaseCreatorCredit> artistCredits;

    public MusicRelease() {}

    public MusicRelease(String name) { setName(name); }

    // ── Content ───────────────────────────────────────────────────────────────

    @Override public ForeignCollection<ReleaseCreatorCredit> getCreatorCredits() { return artistCredits; }
    @Override public void setCreatorCredits(ForeignCollection<ReleaseCreatorCredit> v) { this.artistCredits = v; }

    // ── Getters / setters ─────────────────────────────────────────────────────

    public String      getMbid()                { return mbid; }
    public void        setMbid(String v)        { this.mbid = v; }
    public String      getUpc()                 { return upc; }
    public void        setUpc(String v)         { this.upc = v; }
    public String      getReleaseDate()         { return releaseDate; }
    public void        setReleaseDate(String v) { this.releaseDate = v; }
    public ReleaseType getType()                { return type; }
    public void        setType(ReleaseType v)   { this.type = v; }
    public ForeignCollection<Track>               getTracks()        { return tracks; }
    public ForeignCollection<ReleaseCreatorCredit> getArtistCredits() { return artistCredits; }

    @Override public String toString() { return getName(); }
}
