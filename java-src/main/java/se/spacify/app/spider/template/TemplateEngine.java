package se.spacify.app.spider.template;

import java.io.StringReader;
import java.util.Map;

import javax.xml.parsers.DocumentBuilderFactory;

import org.luaj.vm2.Globals;
import org.luaj.vm2.LuaTable;
import org.luaj.vm2.LuaValue;
import org.luaj.vm2.lib.jse.JsePlatform;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.xml.sax.InputSource;

import se.spacify.app.spider.Request;

/**
 * Renders a Spider template into a DOM {@link Element}. The pipeline is:
 * {@link LuaPreprocessor#compile preprocess} the template to a Lua chunk, run it on
 * a fresh {@link JsePlatform#standardGlobals() Lua sandbox} with the request bound
 * in, then parse the emitted markup as XML.
 *
 * <p>The running chunk sees these globals derived from the {@link Request}:
 * <ul>
 *   <li>{@code method} — the HTTP-style method ({@code "GET"}/{@code "POST"}).</li>
 *   <li>{@code uri} — the request URI.</li>
 *   <li>{@code action} — the postback action name (the {@code onclick} value), or
 *       {@code ""} for a plain load; lets a template branch on what was clicked.</li>
 *   <li>{@code data} — a table of the request's input data (see {@link Request#getData()}).</li>
 *   <li>{@code model} — the controller-supplied model table (see
 *       {@link se.spacify.app.spider.controller.Controller#data(Request)}).</li>
 * </ul>
 */
public final class TemplateEngine {

    private TemplateEngine() {
    }

    /** Compile, run and parse {@code template} against {@code request} with no model, returning its root element. */
    public static Element render(String template, Request request) {
        return render(template, request, null);
    }

    /** Compile, run and parse {@code template} against {@code request} and {@code model}, returning its root element. */
    public static Element render(String template, Request request, Map<String, Object> model) {
        String lua = LuaPreprocessor.compile(template);
        String markup = run(lua, request, model);
        return parse(markup);
    }

    /** Run the compiled Lua chunk with the request and model bound into the globals; returns the emitted markup. */
    private static String run(String luaSource, Request request, Map<String, Object> model) {
        Globals globals = JsePlatform.standardGlobals();
        bindRequest(globals, request);
        globals.set("model", toTable(model));
        LuaValue chunk = globals.load(luaSource, "spider-template");
        return chunk.call().tojstring();
    }

    /** Expose {@code method}, {@code uri}, {@code action} and the {@code data} table to the template. */
    private static void bindRequest(Globals globals, Request request) {
        if (request == null) {
            globals.set("method", "GET");
            globals.set("uri", "");
            globals.set("action", "");
            globals.set("data", new LuaTable());
            return;
        }
        globals.set("method", str(request.getMethod()));
        globals.set("uri", str(request.getUri()));
        globals.set("action", str(action(request)));
        globals.set("data", toTable(request.getData()));
    }

    /** The postback action carried in the {@code action} header, or {@code ""}. */
    private static String action(Request request) {
        Object value = request.getHeaders() != null ? request.getHeaders().get("action") : null;
        return value != null ? value.toString() : "";
    }

    /** Coerce a {@code Map}-shaped data payload into a Lua table of scalar values; {@code {}} otherwise. */
    private static LuaTable toTable(Object data) {
        LuaTable table = new LuaTable();
        if (data instanceof Map<?, ?> map) {
            for (Map.Entry<?, ?> entry : map.entrySet()) {
                table.set(String.valueOf(entry.getKey()), toLua(entry.getValue()));
            }
        }
        return table;
    }

    private static LuaValue toLua(Object value) {
        return switch (value) {
            case null              -> LuaValue.NIL;
            case Number n          -> LuaValue.valueOf(n.doubleValue());
            case Boolean b         -> LuaValue.valueOf(b);
            case Map<?, ?> nested  -> toTable(nested);
            default                -> LuaValue.valueOf(value.toString());
        };
    }

    private static String str(String value) {
        return value != null ? value : "";
    }

    /** Parse rendered markup into its root {@link Element}. */
    private static Element parse(String markup) {
        try {
            DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
            factory.setNamespaceAware(false);
            Document doc = factory.newDocumentBuilder().parse(new InputSource(new StringReader(markup)));
            return doc.getDocumentElement();
        } catch (Exception e) {
            throw new IllegalStateException("Spider template produced invalid XML:\n" + markup, e);
        }
    }
}
