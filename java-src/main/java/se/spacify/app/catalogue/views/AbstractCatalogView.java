package se.spacify.app.catalogue.views;

import se.spacify.navigation.ViewStack;
import se.spacify.app.catalogue.service.MusicCatalogueService;
import se.spacify.app.music.controls.MusicTable;
import se.spacify.app.music.views.AbstractMusicListView;

import javax.swing.JComponent;
import javax.swing.JTextField;
import javax.swing.SwingWorker;
import java.net.URLDecoder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Base for the read-only catalogue browsing views, one per catalogue tree kind
 * (artists / releases / recordings). It reuses {@link AbstractMusicListView}'s
 * themed table, play-queue integration and "Play with…" plumbing, but fetches
 * its rows from a remote {@link MusicCatalogueService} on a background thread —
 * a {@link SwingWorker} per reload, with the latest request winning so quick
 * navigations/searches don't race. A toolbar search field seeds the browse, and
 * the {@code spacify:catalog:<ServiceId>:<kind>?<params>} URI carries the
 * drill-down context (e.g. {@code ?artist=<mbid>}).
 *
 * @param <T> the catalogue entity type a concrete view lists
 */
public abstract class AbstractCatalogView<T> extends AbstractMusicListView {
    protected final List<T> rows = new ArrayList<>();

    private String ServiceId;
    private final Map<String, String> params = new HashMap<>();
    private JTextField search;   // no initializer: assigned in toolbarAccessory() during super()
    private long requestSeq;

    protected AbstractCatalogView(ViewStack viewStack) {
        super(viewStack); 
    }

    @Override protected boolean isEditable() { return false; }

    /** The catalogue tree kind this view serves: {@code artists|releases|recordings}. */
    protected abstract String kind();

    @Override
    public boolean acceptsUri(String uri) {
        return uri != null && uri.matches("spacify:catalog:.+:" + kind() + "(\\?.*)?");
    }

    @Override
    public void navigate(String uri) {
        params.clear();
        ServiceId = null;
        Matcher m = Pattern.compile("spacify:catalog:(.+):" + kind() + "(?:\\?(.*))?").matcher(uri);
        if (m.matches()) {
            ServiceId = m.group(1);
            if (m.group(2) != null) {
                for (String pair : m.group(2).split("&")) {
                    int eq = pair.indexOf('=');
                    if (eq > 0) params.put(pair.substring(0, eq), urlDecode(pair.substring(eq + 1)));
                }
            }
        }
    }

    /** A drill-down parameter from the URI query (e.g. {@code artist}), or null. */
    protected String param(String name) { return params.get(name); }

    /** Current trimmed search text, or "" when the field is empty. */
    protected String query() { return search != null ? search.getText().trim() : ""; }

    /** The catalogue Service named by the current URI, or null if unavailable. */
    protected MusicCatalogueService Service() {
        if (ServiceId == null) return null;
        for (MusicCatalogueService s : getViewStack().getMainWindow().getServiceManager().getServices(MusicCatalogueService.class)) {
            if (ServiceId.equals(s.getId())) return s;
        }
        return null;
    }

    @Override
    protected JComponent toolbarAccessory() {
        search = new JTextField(16);
        search.putClientProperty("JTextField.placeholderText", searchHint());
        search.addActionListener(e -> reload());
        return search;
    }

    protected String searchHint() { return "Search…"; }

    @Override
    protected final void reload() {
        final long seq = ++requestSeq;
        final MusicCatalogueService svc = Service();
        rows.clear();
        musicTable.clear();
        if (svc == null) {
            setHeader("Catalogue unavailable");
            return;
        }
        setHeader(loadingHeader(svc));
        new SwingWorker<List<T>, Void>() {
            @Override protected List<T> doInBackground() {
                try { return fetch(svc); } catch (Exception ex) { return List.of(); }
            }

            @Override protected void done() {
                if (seq != requestSeq) return;   // a newer navigation/search superseded us
                List<T> items;
                try { items = get(); } catch (Exception ex) { items = List.of(); }
                rows.clear();
                musicTable.clear();
                for (T it : items) {
                    rows.add(it);
                    addRow(toRow(it));
                }
                setHeader(items.isEmpty() ? emptyHeader(svc) : resultHeader(svc, items.size()));
                // The managed Buy/Stream and library-toggle columns re-size themselves
                // when the model changes (MusicTable's TableModelListener), so these
                // async rows are covered without an explicit refresh() here.
            }
        }.execute();
    }

    /** Fetch the rows; runs off the EDT, so blocking network I/O belongs here. */
    protected abstract List<T> fetch(MusicCatalogueService svc) throws Exception;

    /** Build the keyed table row for an item (see {@link #row()}); runs on the EDT. */
    protected abstract MusicTable.Row toRow(T item);

    // ── Header text (overridable per view) ──────────────────────────────────────

    protected String loadingHeader(MusicCatalogueService svc) { return svc.getName() + " — loading…"; }
    protected String emptyHeader(MusicCatalogueService svc)   { return svc.getName() + " — no results"; }
    protected String resultHeader(MusicCatalogueService svc, int count) { return svc.getName(); }

    /** Navigate the app to another catalogue URI within the same Service. */
    protected void open(String kindAndQuery) {
        if (getViewStack() != null && ServiceId != null) {
            getViewStack().navigate("spacify:catalog:" + ServiceId + ":" + kindAndQuery);
        }
    }

    private static String urlDecode(String s) {
        return URLDecoder.decode(s, StandardCharsets.UTF_8);
    }
}
