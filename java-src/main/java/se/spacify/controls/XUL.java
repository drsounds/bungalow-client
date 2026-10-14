package se.spacify.controls;

import java.io.InputStream;
import java.io.StringReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Consumer;

import javax.swing.BoxLayout;
import javax.xml.parsers.DocumentBuilderFactory;

import org.w3c.dom.Document;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;

/**
 * A XUL-style element: an XML node that builds a live control tree. Each tag maps
 * to one of the project's {@link Control}s (the Java/Swing translation of the C++
 * {@code SPElement} parser that used {@code wxXml}); container tags lay their
 * children out, leaf tags host a single widget.
 *
 * <pre>{@code
 *   XUL ui = XUL.parse("<vbox><text id='t'>Hello</text><button onclick='go'>OK</button></vbox>");
 *   panel.add(ui.getComponent());
 * }</pre>
 */
public class XUL extends Panel {

	private String tagName = "element";
	/** Hosted leaf control for leaf tags; {@code null} for plain containers. */
	private Control<?> content;
	private final List<XUL> childElements = new ArrayList<>();
	private final Map<String, Consumer<XUL>> listeners = new HashMap<>();

	public XUL() {
		// Containers stack their children vertically by default; leaves override
		// this in setContent().
		getComponent().setLayout(new BoxLayout(getComponent(), BoxLayout.PAGE_AXIS));
		getComponent().setOpaque(false);
	}

	public String getTagName()            { return tagName; }
	public Control<?> getContent()        { return content; }
	/** Where children are appended (here, the element itself). */
	public XUL getSlotElement()           { return this; }
	public List<XUL> getChildElements()   { return childElements; }

	/** Push text content into the hosted widget. */
	public void setInnerText(String text) {
		if (content instanceof Label l)            l.getComponent().setText(text);
		else if (content instanceof TextField t)   t.getComponent().setText(text);
		else if (content instanceof GlossyButton g) g.getComponent().setText(text);
		else if (content instanceof Button b)      b.getComponent().setText(text);
	}

	/**
	 * Register a DOM-style event listener. {@code "click"} on a button maps to the
	 * wrapped button's action.
	 */
	public void addEventListener(String type, Consumer<XUL> handler) {
		listeners.put(type, handler);
		if ("click".equals(type)) {
			if (content instanceof GlossyButton g) g.getComponent().addActionListener(e -> handler.accept(this));
			else if (content instanceof Button b)  b.getComponent().addActionListener(e -> handler.accept(this));
		}
	}

	/** Re-layout this subtree (wx {@code pack()}). */
	public void pack() {
		revalidate();
		repaint();
	}

	// ── Parsing: XML → control tree ──────────────────────────────────────────────

	public static XUL parse(String xml) throws Exception {
		return parse(new InputSource(new StringReader(xml)));
	}

	public static XUL parse(InputStream in) throws Exception {
		return parse(new InputSource(in));
	}

	private static XUL parse(InputSource source) throws Exception {
		DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
		factory.setNamespaceAware(false);
		Document doc = factory.newDocumentBuilder().parse(source);
		XUL root = new XUL();
		return root.parseElement(doc.getDocumentElement());
	}

	/** Build the element subtree for {@code xmlNode}. */
	private XUL parseElement(org.w3c.dom.Element xmlNode) {
		XUL elm = createElement(xmlNode.getTagName());
		elm.setVisible(true);

		NamedNodeMap attrs = xmlNode.getAttributes();
		for (int i = 0; i < attrs.getLength(); i++) {
			Node attribute = attrs.item(i);
			String name = attribute.getNodeName();
			String value = attribute.getNodeValue();
			elm.setAttribute(name, value);
			if ("hidden".equals(name)) {
				elm.setVisible(false);
			}
			if ("onclick".equals(name)) {
				elm.addEventListener("click", elm::onPostBackButtonClick);
			}
		}

		NodeList children = xmlNode.getChildNodes();
		for (int i = 0; i < children.getLength(); i++) {
			Node child = children.item(i);
			if (child.getParentNode() != xmlNode) {
				break;
			}
			if (child.getNodeType() == Node.TEXT_NODE) {
				String text = child.getNodeValue();
				if (text != null && !text.isBlank()) {
					elm.getSlotElement().setInnerText(text.trim());
				}
			} else if (child.getNodeType() == Node.ELEMENT_NODE) {
				XUL childElement = elm.parseElement((org.w3c.dom.Element) child);
				elm.childElements.add(childElement);
				elm.add(childElement);   // mount the child control
				elm.pack();
			}
		}

		elm.pack();
		return elm;
	}

	/**
	 * Map a tag name to a control. Leaf tags host a widget (set as {@link #content});
	 * container tags lay their children out themselves.
	 */
	protected XUL createElement(String tagName) {
		XUL elm = new XUL();
		elm.tagName = tagName;
		switch (tagName) {
			case "text", "img" -> elm.setContent(new Label());      // ~ SPTextElement / SPImageElement
			case "button"      -> elm.setContent(new GlossyButton()); // ~ SPButtonElement
			case "input"       -> elm.setContent(new TextField());   // ~ SPInputElement
			case "view"        -> elm.setContent(new TabbedPane());  // ~ SPTabBarViewElement
			case "splitter"    -> { /* container, handled by SplitPane usage */ }
			case "vbox"        -> elm.getComponent().setLayout(new BoxLayout(elm.getComponent(), BoxLayout.PAGE_AXIS));
			case "hbox"        -> elm.getComponent().setLayout(new BoxLayout(elm.getComponent(), BoxLayout.LINE_AXIS));
			default            -> { /* plain element */ }
		}
		return elm;
	}

	/** Host a single widget as this element's content (leaf tags). */
	private void setContent(Control<?> widget) {
		this.content = widget;
		getComponent().removeAll();
		getComponent().setLayout(new java.awt.BorderLayout());
		if (widget.getComponent() != null) {
			getComponent().add(widget.getComponent(), java.awt.BorderLayout.CENTER);
		}
	}

	/**
	 * Called when an element declaring {@code onclick} is clicked. Override to
	 * handle the postback; no-op by default.
	 */
	protected void onPostBackButtonClick(XUL elm) {
		// no-op
	}
}
