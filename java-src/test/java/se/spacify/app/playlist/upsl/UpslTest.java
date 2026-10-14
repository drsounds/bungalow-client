package se.spacify.app.playlist.upsl;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;

import java.nio.charset.StandardCharsets;

import org.junit.Test;

/**
 * Checks the UPSL Base62 codec (RFC-0001 §4.1) and the {@link Upsl} parser (§5–6),
 * including the spec's worked example (§13).
 */
public class UpslTest {

    @Test
    public void base62RoundTripsUtf8Text() {
        for (String text : new String[] {"", "Roadtrip", "Summer bangers", "åäö — mix", "musik:isrc:GBAAA0000001"}) {
            assertEquals(text, Base62.decodeText(Base62.encodeText(text)));
        }
    }

    @Test
    public void base62PreservesLeadingZeroBytes() {
        byte[] bytes = {0, 0, 0x7f, 0x01};
        assertArrayEquals(bytes, Base62.decode(Base62.encode(bytes)));
    }

    @Test(expected = IllegalArgumentException.class)
    public void base62RejectsNonAlphabetCharacters() {
        Base62.decode("not_base62!");   // '_' and '!' are outside the alphabet
    }

    @Test
    public void parsesEmbeddedLink() {
        String name = "Roadtrip";
        String description = "Summer bangers";
        String uri = "spacify:federated:user:@drsounds@mastodon.social:playlist:8f2bffff-uuid:"
            + Base62.encodeText(name) + ":description:" + Base62.encodeText(description)
            + ":uris:" + Base62.encodeText("musik:isrc:GBAAA0000001");

        Upsl upsl = Upsl.parse(uri);

        assertEquals("@drsounds@mastodon.social", upsl.handle());
        assertEquals("8f2bffff-uuid", upsl.playlistId());
        assertEquals(name, upsl.name());
        assertEquals(description, upsl.description());
        assertEquals(uri, upsl.uri());
    }

    @Test
    public void parsesAcctForm() {
        String name = "Roadtrip";
        String description = "Summer bangers";
        String uri = "acct:drsounds@mastodon.social:playlist:8f2bffff-uuid:"
            + Base62.encodeText(name) + ":description:" + Base62.encodeText(description)
            + ":uris:" + Base62.encodeText("musik:isrc:GBAAA0000001");

        Upsl upsl = Upsl.parse(uri);

        assertEquals("drsounds@mastodon.social", upsl.handle());
        assertEquals("8f2bffff-uuid", upsl.playlistId());
        assertEquals(name, upsl.name());
        assertEquals(description, upsl.description());
        assertEquals(uri, upsl.uri());
    }

    @Test
    public void parsesMusikAcctForm() {
        String name = "Roadtrip";
        String description = "Summer bangers";
        String uri = "musik:acct:drsounds@mastodon.social:playlist:8f2bffff-uuid:"
            + Base62.encodeText(name) + ":description:" + Base62.encodeText(description)
            + ":uris:" + Base62.encodeText("musik:isrc:GBAAA0000001");

        Upsl upsl = Upsl.parse(uri);

        assertEquals("drsounds@mastodon.social", upsl.handle());
        assertEquals("8f2bffff-uuid", upsl.playlistId());
        assertEquals(name, upsl.name());
        assertEquals(description, upsl.description());
        assertEquals(uri, upsl.uri());
    }

    @Test
    public void parsesBungalowAndSpacifyAcctForms() {
        for (String prefix : new String[] {"bungalow:acct:", "spacify:acct:"}) {
            String uri = prefix + "drsounds@mastodon.social:playlist:8f2bffff-uuid:"
                + Base62.encodeText("Roadtrip") + ":description:" + Base62.encodeText("Summer bangers")
                + ":uris:" + Base62.encodeText("musik:isrc:GBAAA0000001");

            Upsl upsl = Upsl.parse(uri);

            assertEquals(prefix, "drsounds@mastodon.social", upsl.handle());
            assertEquals(prefix, "8f2bffff-uuid", upsl.playlistId());
            assertEquals(prefix, "Roadtrip", upsl.name());
            assertEquals(prefix, "Summer bangers", upsl.description());
        }
    }

    @Test
    public void parsesHostedAcctForm() {
        Upsl upsl = Upsl.parse("acct:12345:playlist:8f2bffff-uuid");

        assertEquals("12345", upsl.handle());
        assertEquals("8f2bffff-uuid", upsl.playlistId());
        assertEquals("", upsl.name());
        assertEquals("", upsl.description());
    }

    @Test
    public void descriptionStopsAtImageMarker() {
        String uri = "spacify:federated:user:@a@b:playlist:id:"
            + Base62.encodeText("N") + ":description:" + Base62.encodeText("D")
            + ":image:" + b64u("https://img")
            + ":uris:" + Base62.encodeText("musik:isrc:X");

        Upsl upsl = Upsl.parse(uri);

        assertEquals("N", upsl.name());
        assertEquals("D", upsl.description());
    }

    @Test
    public void hostedLinkHasNoBody() {
        Upsl upsl = Upsl.parse("spacify:federated:user:@a@b:playlist:8f2bffff-uuid");

        assertEquals("8f2bffff-uuid", upsl.playlistId());
        assertEquals("", upsl.name());
        assertEquals("", upsl.description());
    }

    @Test
    public void nonUpslUriYieldsNull() {
        assertNull(Upsl.parse("spacify:testapp"));
        assertNull(Upsl.parse(null));
    }

    /** Minimal Base64url (no padding) — only needed to build a realistic image leaf. */
    private static String b64u(String text) {
        return java.util.Base64.getUrlEncoder().withoutPadding()
            .encodeToString(text.getBytes(StandardCharsets.UTF_8));
    }
}
