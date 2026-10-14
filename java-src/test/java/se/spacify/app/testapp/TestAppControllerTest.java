package se.spacify.app.testapp;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import java.net.URISyntaxException;
import java.util.HashMap;

import org.junit.Test;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;

import se.spacify.app.spider.Request;
import se.spacify.app.testapp.controller.TestController;
import se.spacify.net.Uri;

/**
 * Checks that {@link TestController} routes {@code spacify:testapp} and renders the
 * bundled {@code views/test.xml} template (loaded from the classpath).
 */
public class TestAppControllerTest {

    @Test
    public void acceptsTestAppUriAndRejectsOthers() throws URISyntaxException {
        TestController controller = new TestController();
        assertTrue(controller.acceptsUri(Uri.parse("spacify:testapp")));
        assertTrue(controller.acceptsUri(Uri.parse("spacify:testapp#overview")));
        assertFalse(controller.acceptsUri(Uri.parse("spacify:now-playing")));
    }

    @Test
    public void rendersBundledTemplateFromClasspath() {
        Request request = new Request("GET", "spacify:testapp", new HashMap<>(), null, new HashMap<>());

        Element view = new TestController().process(request);

        assertNotNull("template resource should load and render", view);
        assertEquals("view", view.getTagName());

        Element page = firstDescendant(view, "page");
        assertEquals("overview", page.getAttribute("id"));
        assertEquals("Overview", page.getAttribute("title"));

        // One date line plus the 1..10 loop (counted as descendants, so layout
        // wrappers such as <vbox> around the body don't matter).
        assertEquals(11, page.getElementsByTagName("text").getLength());
        assertEquals(1, page.getElementsByTagName("button").getLength());
    }

    private static Element firstDescendant(Element parent, String tag) {
        NodeList nodes = parent.getElementsByTagName(tag);
        if (nodes.getLength() == 0) {
            throw new AssertionError("no <" + tag + "> element");
        }
        return (Element) nodes.item(0);
    }
}
