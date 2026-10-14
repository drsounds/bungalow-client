package se.spacify.service.media;

import java.util.List;

import se.spacify.app.music.service.MusicService;

/**
 * The availability of a track across installed services, used to render the
 * Buy/Stream cell: whether a local copy exists, and which remote services can
 * stream it. Resolved by {@link AvailabilityResolver} with a cheap, non-network
 * probe ({@code lookup}/{@code lookupByTitleArtist}).
 */
public record TrackAvailability(boolean resolved, boolean local, List<MusicService> streamServices) {

    /** Not-yet-resolved placeholder. */
    public static final TrackAvailability PENDING = new TrackAvailability(false, false, List.of());

    /** Whether any remote service reported it can stream the track. */
    public boolean hasStream() {
        return !streamServices.isEmpty();
    }
}
