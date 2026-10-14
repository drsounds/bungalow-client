package se.spacify.app.search.views;

import se.spacify.navigation.View;
import se.spacify.navigation.ViewStack;
import se.spacify.search.EntityKind;
import se.spacify.search.SearchProvider;
import se.spacify.search.SearchResult;
import se.spacify.service.media.PlaybackCoordinator;
import se.spacify.ui.theme.ThemeManager;

import javax.swing.*;
import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.net.URLDecoder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

/**
 * The unified search screen ({@code spacify:search?q=…}). It queries every
 * registered {@link SearchProvider} off the EDT — streaming each provider's
 * results in as they arrive — and presents them grouped into sections by
 * {@link EntityKind}. A result navigates to its view, or plays if it's a
 * playback URI (e.g. a YouTube video).
 */
public class SearchView extends View {

    private final JLabel queryLabel;
    private final JPanel resultsPanel;
    private final List<SearchResult> all = new ArrayList<>();

    private SwingWorker<Void, List<SearchResult>> worker;
    private int seq;

    public SearchView(ViewStack viewStack) {
        super(viewStack);
        getComponent().setLayout(new BorderLayout(0, 12));
        getComponent().setOpaque(false);
        getComponent().setBorder(BorderFactory.createEmptyBorder(20, 20, 20, 20));

        queryLabel = new JLabel("Search");
        queryLabel.setFont(queryLabel.getFont().deriveFont(Font.BOLD, 20f));
        queryLabel.setForeground(Color.WHITE);
        getComponent().add(queryLabel, BorderLayout.NORTH);

        resultsPanel = new JPanel();
        resultsPanel.setLayout(new BoxLayout(resultsPanel, BoxLayout.Y_AXIS));
        resultsPanel.setOpaque(false);

        JScrollPane scroll = new JScrollPane(resultsPanel);
        scroll.setOpaque(false);
        scroll.getViewport().setOpaque(false);
        scroll.setBorder(BorderFactory.createEmptyBorder());
        getComponent().add(scroll, BorderLayout.CENTER);
    }

    @Override
    public boolean acceptsUri(String uri) {
        return uri != null && uri.matches("spacify:search.*");
    }

    @Override
    public void navigate(String uri) {
        String query = "";
        if (uri.contains("?q=")) {
            query = URLDecoder.decode(uri.substring(uri.indexOf("?q=") + 3), StandardCharsets.UTF_8);
        }
        queryLabel.setText(query.isEmpty() ? "Search" : "Results for: " + query);

        // Supersede any in-flight search.
        final int mySeq = ++seq;
        if (worker != null) worker.cancel(true);
        all.clear();
        render();

        if (query.isBlank()) return;

        List<SearchProvider> providers = new ArrayList<>(getMainWindow().getSearchManager().providers());
        final String q = query;
        worker = new SwingWorker<>() {
            @Override protected Void doInBackground() {
                for (SearchProvider p : providers) {
                    if (isCancelled()) break;
                    try {
                        List<SearchResult> rs = p.search(q);
                        if (rs != null && !rs.isEmpty()) publish(rs);
                    } catch (Exception ignored) { /* one provider failing is fine */ }
                }
                return null;
            }
            @Override protected void process(List<List<SearchResult>> chunks) {
                if (mySeq != seq) return;   // a newer search started
                for (List<SearchResult> rs : chunks) all.addAll(rs);
                render();
            }
        };
        worker.execute();
    }

    /** Rebuild the grouped sections from {@link #all}. */
    private void render() {
        resultsPanel.removeAll();
        all.stream()
           .map(SearchResult::kind)
           .distinct()
           .sorted(Comparator.comparingInt(EntityKind::order))
           .forEach(kind -> {
               resultsPanel.add(sectionHeader(kind.label()));
               for (SearchResult r : all) {
                   if (r.kind() == kind) resultsPanel.add(resultRow(r));
               }
           });
        if (all.isEmpty()) {
            JLabel none = new JLabel("No results");
            none.setForeground(new Color(150, 150, 150));
            none.setBorder(BorderFactory.createEmptyBorder(8, 0, 0, 0));
            resultsPanel.add(none);
        }
        resultsPanel.revalidate();
        resultsPanel.repaint();
    }

    private static JLabel sectionHeader(String text) {
        JLabel l = new JLabel(text);
        l.setFont(l.getFont().deriveFont(Font.BOLD, 14f));
        l.setForeground(new Color(180, 180, 180));
        l.setBorder(BorderFactory.createEmptyBorder(12, 0, 4, 0));
        l.setAlignmentX(java.awt.Component.LEFT_ALIGNMENT);
        return l;
    }

    private JComponent resultRow(SearchResult r) {
        String text = "<html>" + escape(r.title())
            + (r.subtitle().isEmpty() ? "" : " <font color='#999999'>— " + escape(r.subtitle()) + "</font>")
            + (r.source().isEmpty() ? "" : "  <font color='#777777'>[" + escape(r.source()) + "]</font>")
            + "</html>";
        JLabel row = new JLabel(text);
        row.setForeground(new Color(220, 220, 220));
        row.setBorder(BorderFactory.createEmptyBorder(5, 8, 5, 8));
        row.setAlignmentX(java.awt.Component.LEFT_ALIGNMENT);
        row.setMaximumSize(new Dimension(Integer.MAX_VALUE, row.getPreferredSize().height + 10));
        if (r.uri() != null && !r.uri().isBlank()) {
            row.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
            row.addMouseListener(new MouseAdapter() {
                @Override public void mouseClicked(MouseEvent e) { activate(r); }
                @Override public void mouseEntered(MouseEvent e) { row.setForeground(ThemeManager.getAccentBackgroundColor()); }
                @Override public void mouseExited(MouseEvent e)  { row.setForeground(new Color(220, 220, 220)); }
            });
        }
        return row;
    }

    /** Navigate the result's URI, or play it when it's a playback URI. */
    private void activate(SearchResult r) {
        String uri = r.uri();
        if (uri == null || uri.isBlank()) return;
        if (uri.startsWith("spacify:youtube:") || uri.startsWith("spacify:local:")) {
            PlaybackCoordinator.playUri(uri);
        } else {
            getViewStack().navigate(uri);
        }
    }

    private static String escape(String s) {
        return s == null ? "" : s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;");
    }

    @Override
    public String getName() { return "Search"; }
}
