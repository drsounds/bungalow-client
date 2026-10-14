package se.spacify.service.media;

import com.j256.ormlite.dao.Dao;
import com.j256.ormlite.stmt.Where;
import se.spacify.db.DatabaseManager;
import se.spacify.app.media.model.MusicServiceTrack;

import se.spacify.app.music.model.Track;

import java.util.List;

/**
 * Reads and writes the user's persisted "Play with…" choices
 * ({@link MusicServiceTrack}). A choice is found by the local {@link Track}
 * foreign key when the play came from the library, otherwise by the request's
 * stable {@link PlayRequest#trackKey()} — the "Both" keying strategy.
 */
final class ResolutionStore {

    private ResolutionStore() {}

    private static Dao<MusicServiceTrack, Integer> dao() {
        return DatabaseManager.getInstance().dao(MusicServiceTrack.class);
    }

    /** The saved choice for this request, or {@code null} if none / on error. */
    static MusicServiceTrack find(PlayRequest req) {
        try {
            Where<MusicServiceTrack, Integer> where = dao().queryBuilder().where();
            if (req.track() != null) {
                where.eq("track_id", req.track().getId());
            } else {
                where.eq("trackKey", req.trackKey());
            }
            List<MusicServiceTrack> hits = where.query();
            return hits.isEmpty() ? null : hits.get(0);
        } catch (Exception e) {
            return null;
        }
    }

    /**
     * Persist (upserting) the user's pick for this request: any prior choice for
     * the same track/key is replaced. Records both the {@link Track} FK (when
     * present) and the stable key so the choice resolves from any play source.
     */
    static void save(PlayRequest req, ServiceMatch match) {
        try {
            clear(req);
            MusicServiceTrack m = new MusicServiceTrack();
            m.setTrack(req.track());
            m.setTrackKey(req.trackKey());
            m.setServiceId(match.ServiceId());
            // Record how the Service resolved it so the saved pick replays the
            // same way without re-searching: by ISRC, else by title/artist.
            if (match.byIsrc()) m.setMatchIsrc(req.isrc());
            m.setMatchTitle(req.title());
            m.setMatchArtist(req.artist());
            // The concrete candidate target (e.g. the chosen YouTube video) so the
            // saved pick replays exactly, not as a re-search.
            if (match.uri() != null) m.setMatchUri(match.uri());
            dao().create(m);
        } catch (Exception ignored) {
        }
    }

    /** Remove any saved choice bound to this request's track / key. */
    static void clear(PlayRequest req) {
        try {
            Dao<MusicServiceTrack, Integer> dao = dao();
            if (req.track() != null) {
                dao.delete(dao.queryForEq("track_id", req.track().getId()));
            }
            dao.delete(dao.queryForEq("trackKey", req.trackKey()));
        } catch (Exception ignored) {
        }
    }
}
