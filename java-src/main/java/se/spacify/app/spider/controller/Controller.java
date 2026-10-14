package se.spacify.app.spider.controller;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Map;
import java.util.stream.Collectors;

import org.w3c.dom.Element;

import se.spacify.app.spider.Request;
import se.spacify.app.spider.template.TemplateEngine;
import se.spacify.net.Uri;

/**
 * Handles the Spider requests it {@link #acceptsUri accepts}, turning each into a
 * control tree. A controller supplies a Lua-preprocessor template (from a file,
 * resource or string) via {@link #template(Request)} and the model backing it via
 * {@link #data(Request)}; {@link #process(Request)} renders the template with the
 * {@link TemplateEngine} — binding the model as the {@code model} global, then
 * interpolating {@code ${...}} expressions and running {@code %}-directives — and
 * returns the resulting {@link Element} tree for the
 * {@link se.spacify.app.spider.views.SpiderView} to mount.
 *
 * <p>Postbacks re-enter the same controller: a button {@code onclick="refresh"}
 * sends a {@code POST} whose {@code action} header is {@code "refresh"} (readable
 * in the template as the {@code action} global), so a single template can branch on
 * what was clicked and re-render.
 *
 * <pre>{@code
 *   class OverviewController extends Controller {
 *       public boolean acceptsUri(Uri uri) { return "spacify".equals(uri.getScheme()); }
 *       protected String template(Request request) {
 *           return """
 *               <view>
 *                 <page id="overview" title="${model.title}">
 *                   <text>${os.date("%Y-%m-%d")}</text>
 *                   % for i,10 do
 *                   <text>${i}</text>
 *                   % end
 *                   <button onclick="refresh">Refresh</button>
 *                 </page>
 *               </view>""";
 *       }
 *       protected Map<String, Object> data(Request request) {
 *           return Map.of("title", "Overview");
 *       }
 *   }
 * }</pre>
 */
public abstract class Controller {
/**
     * Reads given resource file as a string.
     *
     * @param fileName path to the resource file
     * @return the file's contents
     * @throws IOException if read fails for any reason
     */ 
    public static String getResourceFileAsString(String fileName) throws IOException {
        ClassLoader classLoader = ClassLoader.getSystemClassLoader();
        try (InputStream is = classLoader.getResourceAsStream(fileName)) {
            if (is == null) return null;
            try (InputStreamReader isr = new InputStreamReader(is);
                BufferedReader reader = new BufferedReader(isr)) {
                return reader.lines().collect(Collectors.joining(System.lineSeparator()));
            }
        }
    }
    public abstract boolean acceptsUri(Uri uri);

    /**
     * The Lua-preprocessor template for this request — loaded from a file, classpath
     * resource or built inline. See the class javadoc for the directive syntax.
     */
    protected abstract String template(Request request);

    /**
     * The model backing this request's {@link #template(Request) template}: the values
     * the Lua preprocessor may read through the {@code model} global (e.g.
     * {@code ${model.title}}). Entries are coerced to Lua scalars, with nested
     * {@code Map}s becoming nested tables. Return an empty map to expose no model.
     */
    protected abstract Map<String, Object> data(Request request);

    /** Render this controller's {@link #template(Request) template} against the request. */
    public Element process(Request request) {
        return TemplateEngine.render(template(request), request, data(request));
    }
}
