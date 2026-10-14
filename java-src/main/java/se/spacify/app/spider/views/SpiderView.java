package se.spacify.app.spider.views;

import java.awt.BorderLayout;
import java.util.HashMap;
import java.util.Map;

import javax.xml.parsers.DocumentBuilderFactory;

import org.w3c.dom.Document;
import org.w3c.dom.Element;

import se.spacify.app.spider.Postbacks;
import se.spacify.app.spider.Request;
import se.spacify.app.spider.Spider;
import se.spacify.controls.XUL;

/**
 * The on-screen surface for a {@link Spider} application. It {@link #request(Request)
 * issues a request}, asks the spider to {@link Spider#process process} it into an
 * {@link Element} tree, and renders that tree into a live control subtree via
 * {@link Control#setInnerXul(Element) setInnerXul} — so re-rendering after a postback
 * reconciles against the previous tree and preserves widget state (selected tab,
 * caret, focus).
 *
 * <p>Buttons declaring {@code onclick} post back: clicking one re-issues a {@code POST}
 * carrying the action name and the current input fields, and the view re-renders.
 */
public class SpiderView extends XUL {

    private Spider spider = new Spider();

    /** The URI of the last request, replayed by postbacks so they hit the same controller. */
    private String currentUri = "";

    public Spider getSpider() {
        return spider;
    }

    public SpiderView() {
        super();
        // A view fills its slot; one rendered root control sits at the centre.
        getComponent().setLayout(new BorderLayout());
    }

    /** Load a URI fresh (a {@code GET}); the controller renders the initial tree. */
    public void navigate(String uri) {
        request(new Request("GET", uri, new HashMap<>(), null, new HashMap<>()));
    }

    /**
     * Issue {@code request} to the spider and render the resulting tree. Triggered
     * both by {@link #navigate(String)} and by button postbacks.
     */
    public void request(Request request) {
        currentUri = request.getUri();
        Element tree = spider.process(request);
        render(tree);
    }

    /** Re-issue the last URI as a {@code POST} carrying {@code action} and the current input fields. */
    public void postBack(String action) {
        Map<String, Object> headers = new HashMap<>();
        headers.put("action", action);
        request(new Request("POST", currentUri, headers, action, Postbacks.collectInput(this)));
    }

    // ── Rendering ────────────────────────────────────────────────────────────────

    private void render(Element root) {
        if (root == null) {
            return;
        }
        // setInnerXul renders a root's *children*; wrap so the root element itself
        // (e.g. <view> → TabbedPane) becomes this view's single reconciled child.
        setInnerXul(wrap(root));
        Postbacks.bind(this, this::postBack);
        revalidate();
        repaint();
    }

    /** Place {@code root} under a synthetic parent in a fresh document so it renders as one child. */
    private static Element wrap(Element root) {
        try {
            DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
            factory.setNamespaceAware(false);
            Document doc = factory.newDocumentBuilder().newDocument();
            Element wrapper = doc.createElement("spider-root");
            wrapper.appendChild(doc.importNode(root, true));
            doc.appendChild(wrapper);
            return wrapper;
        } catch (Exception e) {
            throw new IllegalStateException("Failed to wrap Spider root element", e);
        }
    }
}
