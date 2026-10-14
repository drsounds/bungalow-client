package se.spacify.app.music.model;

import com.j256.ormlite.field.DatabaseField;
import com.j256.ormlite.table.DatabaseTable;

/** Credits an {@link Artist} on a {@link MusicRelease}, with role metadata. */
@DatabaseTable(tableName = "release_artist_credits")
public class ReleaseCreatorCredit extends CreatorCredit<MusicRelease> {

    @DatabaseField(foreign = true, foreignAutoRefresh = true, canBeNull = false, columnName = "release_id")
    private MusicRelease release;

    @DatabaseField(foreign = true, foreignAutoRefresh = true, canBeNull = false, columnName = "artist_id")
    private Artist artist;

    public ReleaseCreatorCredit() {}

    public ReleaseCreatorCredit(MusicRelease release, Artist artist, boolean primary, String role) {
        this.release = release;
        this.artist  = artist;
        setPrimary(primary);
        setRole(role);
    }

    public MusicRelease getRelease()              { return release; }
    public void         setRelease(MusicRelease v) { this.release = v; }
    public Artist       getArtist()               { return artist; }
    public void         setArtist(Artist v)       { this.artist = v; }

    @Override public Creator      getCreator() { return artist; }
    @Override public MusicRelease getContent() { return release; }
}
