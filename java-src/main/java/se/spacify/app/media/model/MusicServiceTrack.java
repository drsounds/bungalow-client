package se.spacify.app.media.model;

import se.spacify.app.music.model.*;
import com.j256.ormlite.field.DatabaseField;
import com.j256.ormlite.table.DatabaseTable;

/**
 * A user's saved decision about <em>which music Service</em> plays a given track
 * — the persistence behind the "Play with…" chooser (an "Open with" for music).
 *
 * <p>The choice is keyed two ways so it survives both library and non-library
 * plays: a nullable foreign key to the local {@link Track} when the play
 * originated from the library, and always a stable {@link #trackKey} (the track's
 * ISRC or play URI) so catalogue/transient plays can be matched too. The
 * {@code Service*}/{@code match*} fields record enough to replay the pick on the
 * chosen Service without re-running the cross-Service search.
 */
@DatabaseTable(tableName = "music_Service_tracks")
public class MusicServiceTrack {

    @DatabaseField(generatedId = true)
    private int id;

    /** The library track this pick is bound to, when the play came from the library. */
    @DatabaseField(foreign = true, foreignAutoRefresh = true, canBeNull = true, columnName = "track_id")
    private Track track;

    /** Stable track identity (ISRC or play URI) used when there is no {@link #track}. */
    @DatabaseField(canBeNull = true, index = true)
    private String trackKey;

    /** {@code getServiceId()} of the chosen {@link se.spacify.app.music.service.MusicService}. */
    @DatabaseField(canBeNull = false)
    private String ServiceId;

    // ── Chosen match descriptor (enough to replay without re-searching) ─────────

    @DatabaseField(canBeNull = true) private String matchIsrc;
    @DatabaseField(canBeNull = true) private String matchTitle;
    @DatabaseField(canBeNull = true) private String matchArtist;
    @DatabaseField(canBeNull = true) private String matchUri;

    public MusicServiceTrack() {}

    public int     getId()                 { return id; }
    public Track   getTrack()              { return track; }
    public void    setTrack(Track v)       { this.track = v; }
    public String  getTrackKey()           { return trackKey; }
    public void    setTrackKey(String v)   { this.trackKey = v; }
    public String  getServiceId()          { return ServiceId; }
    public void    setServiceId(String v)  { this.ServiceId = v; }
    public String  getMatchIsrc()          { return matchIsrc; }
    public void    setMatchIsrc(String v)  { this.matchIsrc = v; }
    public String  getMatchTitle()         { return matchTitle; }
    public void    setMatchTitle(String v) { this.matchTitle = v; }
    public String  getMatchArtist()        { return matchArtist; }
    public void    setMatchArtist(String v){ this.matchArtist = v; }
    public String  getMatchUri()           { return matchUri; }
    public void    setMatchUri(String v)   { this.matchUri = v; }
}
