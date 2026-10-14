package se.spacify.app.spider;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.junit.Test;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

import se.spacify.app.spider.controller.Controller;
import se.spacify.net.Uri;

/**
 * End-to-end checks for the Spider template pipeline: a controller's Lua template is
 * preprocessed, run, and parsed into a DOM tree by {@link Spider#process(Request)}.
 */
public class SpiderTemplateTest {

    /** A controller that renders the README example (with small, assertable counts). */
    private static final class OverviewController extends Controller {
        @Override
        public boolean acceptsUri(Uri uri) {
            return "spacify".equals(uri.getScheme());
        }

        @Override
        protected String template(Request request) {
            return """
                <view>
                    <page id="overview" title="${model.title}">
                        <text>${string.upper("hi")}</text>
                        % for i,3 do
                        <text>${i}</text>
                        % end
                        <text>${action}</text>
                        <button onclick="refresh">Refresh</button>
                    </page>
                </view>""";
        }

        @Override
        protected Map<String, Object> data(Request request) {
            return Map.of("title", "Overview");
        }
    }

    private Spider spider() {
        Spider spider = new Spider();
        spider.getControllers().add(new OverviewController());
        return spider;
    }

    @Test
    public void rendersTemplateIntoElementTree() {
        Request request = new Request("GET", "spacify:overview", new HashMap<>(), null, new HashMap<>());

        Element view = spider().process(request);

        assertNotNull("a registered controller should handle the URI", view);
        assertEquals("view", view.getTagName());

        Element page = firstChild(view, "page");
        assertEquals("overview", page.getAttribute("id"));
        assertEquals("Overview", page.getAttribute("title"));

        List<Element> texts = children(page, "text");
        // interpolation + three loop iterations + the action line
        assertEquals(5, texts.size());
        assertEquals("HI", texts.get(0).getTextContent().trim());
        assertEquals("1", texts.get(1).getTextContent().trim());
        assertEquals("2", texts.get(2).getTextContent().trim());
        assertEquals("3", texts.get(3).getTextContent().trim());
        assertEquals("", texts.get(4).getTextContent().trim()); // no action on a GET

        Element button = firstChild(page, "button");
        assertEquals("refresh", button.getAttribute("onclick"));
    }

    @Test
    public void postbackActionIsVisibleToTemplate() {
        HashMap<String, Object> headers = new HashMap<>();
        headers.put("action", "refresh");
        Request post = new Request("POST", "spacify:overview", headers, "refresh", new HashMap<>());

        Element view = spider().process(post);
        Element page = firstChild(view, "page");
        List<Element> texts = children(page, "text");

        assertEquals("refresh", texts.get(4).getTextContent().trim());
    }

    @Test
    public void unroutedUriYieldsNull() {
        Request request = new Request("GET", "http:example.com", new HashMap<>(), null, new HashMap<>());
        assertEquals(null, spider().process(request));
    }

    // ── helpers ──────────────────────────────────────────────────────────────────

    private static Element firstChild(Element parent, String tag) {
        List<Element> matches = children(parent, tag);
        assertNotNull("expected a <" + tag + "> child", matches.isEmpty() ? null : matches);
        return matches.get(0);
    }

    private static List<Element> children(Element parent, String tag) {
        List<Element> out = new ArrayList<>();
        NodeList nodes = parent.getChildNodes();
        for (int i = 0; i < nodes.getLength(); i++) {
            Node node = nodes.item(i);
            if (node.getNodeType() == Node.ELEMENT_NODE && ((Element) node).getTagName().equals(tag)) {
                out.add((Element) node);
            }
        }
        return out;
    }
}
