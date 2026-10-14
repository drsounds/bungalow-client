package se.spacify.app.music.net;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

import se.spacify.app.music.net.MusikUri.Kind;
import se.spacify.service.media.PlayRequest;

/** Round-trip parse/build and playback-resolution tests for {@link MusikUri} (RFC-0002). */
public class MusikUriTest {

    // ── Identifier forms ─────────────────────────────────────────────────────

    @Test public void isrcRoundTrips() {
        MusikUri m = MusikUri.parse("musik:isrc:GBAYE0601498");
        assertNotNull(m);
        assertEquals(Kind.ISRC, m.kind());
        assertEquals("GBAYE0601498", m.id());
        assertEquals("musik:isrc:GBAYE0601498", m.toString());
    }

    @Test public void isrcIsNormalized() {
        // separators stripped, upper-cased
        assertEquals("musik:isrc:GBAYE0601498", MusikUri.isrc("gb-aye-06-01498").toString());
    }

    @Test public void otherStandardIds() {
        assertEquals(Kind.ISWC, MusikUri.parse("musik:iswc:T0345246800").kind());
        assertEquals(Kind.UPC,  MusikUri.parse("musik:upc:00602557091936").kind());
        assertEquals(Kind.ISNI, MusikUri.parse("musik:isni:000000012146438X").kind());
        assertEquals(Kind.IPI,  MusikUri.parse("musik:ipi:00016860138").kind());
    }

    @Test public void genreAndMoodAreSlugged() {
        assertEquals("musik:genre:drum-bass", MusikUri.genre("Drum & Bass").toString());
        assertEquals("musik:mood:melancholic", MusikUri.mood("Melancholic").toString());
        MusikUri g = MusikUri.parse("musik:genre:drum-bass");
        assertEquals(Kind.GENRE, g.kind());
        assertEquals("drum-bass", g.id());
    }

    // ── Name form ────────────────────────────────────────────────────────────

    @Test public void artistOnly() {
        MusikUri m = MusikUri.parse("musik:artist:Aphex%20Twin");
        assertEquals(Kind.ARTIST, m.kind());
        assertEquals("Aphex Twin", m.artist());
        assertEquals("musik:artist:Aphex%20Twin", m.toString());
    }

    @Test public void release() {
        MusikUri m = MusikUri.parse("musik:artist:Aphex%20Twin:release:Drukqs");
        assertEquals(Kind.RELEASE, m.kind());
        assertEquals("Drukqs", m.release());
        assertEquals("musik:artist:Aphex%20Twin:release:Drukqs", m.toString());
    }

    @Test public void recordingWithPositionAndVersion() {
        String uri = "musik:artist:Radiohead:release:OK%20Computer:track:5:name:Karma%20Police:version:Live";
        MusikUri m = MusikUri.parse(uri);
        assertEquals(Kind.RECORDING, m.kind());
        assertEquals("Radiohead", m.artist());
        assertEquals("OK Computer", m.release());
        assertEquals(5, m.trackNumber());
        assertEquals("Karma Police", m.recording());
        assertEquals("Live", m.version());
        assertEquals(uri, m.toString());   // round-trips exactly
    }

    @Test public void recordingWithoutTrackNumber() {
        MusikUri m = MusikUri.parse("musik:artist:Radiohead:release:OK%20Computer:name:Karma%20Police");
        assertEquals(Kind.RECORDING, m.kind());
        assertEquals(0, m.trackNumber());
        assertEquals("Karma Police", m.recording());
    }

    @Test public void encodedDelimitersSurviveRoundTrip() {
        // A colon inside a name must not be read as a delimiter.
        MusikUri built = MusikUri.recording("AC/DC", "Who Made Who: OST", 3, "D.T.", null);
        MusikUri reparsed = MusikUri.parse(built.toString());
        assertEquals("AC/DC", reparsed.artist());
        assertEquals("Who Made Who: OST", reparsed.release());
        assertEquals("D.T.", reparsed.recording());
    }

    // ── Query form ───────────────────────────────────────────────────────────

    @Test public void queryFormResolvesLikeNameForm() {
        MusikUri q = MusikUri.parse(
            "musik:track?name=Karma%20Police&artist_name=Radiohead&release_name=OK%20Computer&number=5");
        assertEquals(Kind.RECORDING, q.kind());
        assertEquals("Karma Police", q.recording());
        assertEquals("Radiohead", q.artist());
        assertEquals("OK Computer", q.release());
        assertEquals(5, q.trackNumber());
    }

    @Test public void queryFormRequiresName() {
        assertNull(MusikUri.parse("musik:track?artist_name=Radiohead"));
    }

    @Test public void recordingWithoutReleaseRendersAsQuery() {
        MusikUri m = MusikUri.recording("Radiohead", null, 0, "Karma Police", null);
        assertTrue(m.toString().startsWith("musik:track?name=Karma%20Police"));
    }

    // ── Legacy compatibility ─────────────────────────────────────────────────

    @Test public void legacyRecordingUriParsesAsIsrc() {
        MusikUri m = MusikUri.parse("spacify:recording:isrc:GBAYE0601498");
        assertEquals(Kind.ISRC, m.kind());
        assertEquals("GBAYE0601498", m.id());
        assertEquals("GBAYE0601498", MusikUri.isrcOf("spacify:recording:isrc:GBAYE0601498"));
    }

    @Test public void isContentUri() {
        assertTrue(MusikUri.isContentUri("musik:isrc:X"));
        assertTrue(MusikUri.isContentUri("spacify:recording:isrc:X"));
        assertTrue(!MusikUri.isContentUri("spacify:playlist:123"));
        assertTrue(!MusikUri.isContentUri(null));
    }

    // ── Playback resolution ──────────────────────────────────────────────────

    @Test public void isrcToPlayRequest() {
        PlayRequest req = MusikUri.toPlayRequest("musik:isrc:GBAYE0601498");
        assertNotNull(req);
        assertEquals("GBAYE0601498", req.isrc());
        assertEquals("musik:isrc:GBAYE0601498", req.fallbackUri());
    }

    @Test public void recordingToPlayRequestUsesMetadata() {
        PlayRequest req = MusikUri.toPlayRequest(
            "musik:artist:Radiohead:release:OK%20Computer:track:5:name:Karma%20Police");
        assertNotNull(req);
        assertNull(req.isrc());
        assertEquals("Karma Police", req.title());
        assertEquals("Radiohead", req.artist());
    }

    @Test public void nonPlayableFormsHaveNoPlayRequest() {
        assertNull(MusikUri.toPlayRequest("musik:artist:Radiohead"));
        assertNull(MusikUri.toPlayRequest("musik:genre:rock"));
        assertNull(MusikUri.toPlayRequest("musik:upc:00602557091936"));
        assertNull(MusikUri.toPlayRequest("spacify:playlist:123"));
    }

    // ── Malformed input ──────────────────────────────────────────────────────

    @Test public void malformedReturnsNull() {
        assertNull(MusikUri.parse(null));
        assertNull(MusikUri.parse("musik:"));
        assertNull(MusikUri.parse("musik:isrc:"));
        assertNull(MusikUri.parse("https://example.com"));
        assertNull(MusikUri.parse("musik:bogus:x"));
    }
}
