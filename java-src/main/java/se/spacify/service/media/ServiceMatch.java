package se.spacify.service.media;

import se.spacify.app.music.service.MusicService;
import se.spacify.app.music.service.MusicService.Candidate;

import javax.swing.ImageIcon;

/**
 * A single candidate in the "Play with…" chooser: a {@link MusicService} together
 * with one concrete {@link Candidate} it can play. A service may contribute
 * several (e.g. YouTube returns one per search result), and a candidate's
 * {@link #uri()} lets the pick play that exact item later without re-searching.
 */
public final class ServiceMatch {

    private final MusicService Service;
    private final Candidate    candidate;

    public ServiceMatch(MusicService Service, Candidate candidate) {
        this.Service   = Service;
        this.candidate = candidate;
    }

    public MusicService Service()   { return Service; }
    public Candidate    candidate() { return candidate; }

    /** Concrete play target (e.g. {@code spacify:youtube:<id>}), or null. */
    public String  uri()    { return candidate.uri(); }
    /** Human label for this candidate (the matched/video title). */
    public String  label()  { return candidate.title(); }
    public String  artist() { return candidate.artist(); }

    /** Whether the Service matched on ISRC (vs. title/artist) — drives how it loads. */
    public boolean byIsrc() { return candidate.byIsrc(); }

    public String    ServiceId()   { return Service.getId(); }
    public String    ServiceName() { return Service.getName(); }
    public ImageIcon icon()        { return Service.getServiceIcon(); }
}
