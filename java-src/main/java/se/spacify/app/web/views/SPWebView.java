package se.spacify.app.web.views;

import org.cef.CefClient;
import org.cef.browser.CefBrowser;
import org.cef.browser.CefFrame;
import org.cef.handler.CefDisplayHandlerAdapter;
import se.spacify.navigation.View;
import se.spacify.navigation.ViewStack;
import se.spacify.app.downloads.DownloadService;
import se.spacify.ui.theme.ThemeManager;
import se.spacify.web.BookmarkEvents;
import se.spacify.web.BookmarkManager;
import se.spacify.web.CefRuntime;
import se.spacify.web.SiteUri;

import javax.swing.*;
import java.awt.*;

/**
 * An embedded-browser view for {@code spacify:site:<host>[:path]} URIs.
 *
 * <p>Navigation is bilateral: a spacify URI is translated to https and loaded in
 * CEF; conversely, link clicks / in-page history inside CEF are translated back
 * to {@code spacify:site:} URIs and reflected into the address bar and nav
 * buttons via {@link ViewStack}. While a site is open the app's back/forward
 * drive the browser's own history (see {@link #handlesHistory()}).
 *
 * <p>CEF is initialised lazily on first navigation (off the EDT, since the first
 * run downloads the native Chromium bundle).
 */
public class SPWebView extends View {

    private final JLabel      status;

    private CefClient  client;
    private CefBrowser browser;
    private boolean    initStarted = false;
    private String     pendingUrl;
	private JToolBar toolbar;
	private JButton btnRefresh;
    private JButton    bookmarkBtn;
    private JTextField uriField;
    private String     currentUri;
    private String     currentTitle;

    public SPWebView(ViewStack viewStack) {
        super(viewStack);
        getComponent().setLayout(new BorderLayout());
        status = new JLabel("", SwingConstants.CENTER);
        status.setBorder(BorderFactory.createEmptyBorder(24, 24, 24, 24));
        getComponent().add(status, BorderLayout.CENTER);
        updateColors();
        ThemeManager.addChangeListener(this::updateColors);
    }

    // ── Overridable scheme hooks (SPServiceWebView uses spacify:store:) ──────────

    /** The spacify URI prefix this view handles. */
    protected String prefix() { return SiteUri.PREFIX; }

    /** Whether the toolbar shows the bookmark toggle. */
    protected boolean supportsBookmarks() { return true; }

    // ── SPView ──────────────────────────────────────────────────────────────────

    @Override public boolean acceptsUri(String uri) { return SiteUri.hasPrefix(uri, prefix()); }

    @Override
    public void navigate(String uri) {
        String url = SiteUri.toUrl(uri, prefix());
        if (url == null) return;
        currentUri = uri;
        if (uriField != null) uriField.setText(uri);
        updateStar();
        if (browser == null) {
            pendingUrl = url;
            ensureBrowser();
        } else if (!url.equals(browser.getURL())) {
            browser.loadURL(url);
        }
    }

    @Override public String getName() { return "Web"; }

    // ── History delegation (app ◄ ► drive the browser) ───────────────────────────

    @Override public boolean handlesHistory() { return browser != null; }
    @Override public boolean canGoBack()      { return browser != null && browser.canGoBack(); }
    @Override public boolean canGoForward()   { return browser != null && browser.canGoForward(); }
    @Override public void    goBack()         { if (browser != null) browser.goBack(); }
    @Override public void    goForward()      { if (browser != null) browser.goForward(); }

    // ── CEF lifecycle ─────────────────────────────────────────────────────────────

    private void ensureBrowser() {
        if (initStarted) return;
        initStarted = true;
        status.setText("Starting browser… (first run downloads Chromium)");

        new SwingWorker<CefClient, Void>() {
            @Override protected CefClient doInBackground() throws Exception {
                return CefRuntime.newClient();   // heavy: builds CefApp on first call
            }
            @Override protected void done() {
                try {
                    client = get();
                    attachBrowser(pendingUrl != null ? pendingUrl : "about:blank");
                } catch (Exception e) {
                    status.setText("<html>Could not start the embedded browser:<br>"
                        + e.getMessage() + "</html>");
                }
            }
        }.execute();
    }

    private void attachBrowser(String url) {
        client.addDisplayHandler(new CefDisplayHandlerAdapter() {
            @Override
            public void onAddressChange(CefBrowser b, CefFrame frame, String newUrl) {
                if (frame == null || !frame.isMain()) return;
                String sp = SiteUri.toSpacifyUri(newUrl, prefix());
                if (sp == null) return;
                SwingUtilities.invokeLater(() -> {
                    currentUri = sp;
                    if (uriField != null) uriField.setText(sp);
                    updateStar();
                    getViewStack().updateCurrentUri(sp);
                });
            }
            @Override
            public void onTitleChange(CefBrowser b, String title) {
                SwingUtilities.invokeLater(() -> currentTitle = title);
            }
        });

        // Capture audio downloads (incl. paid store downloads) into the Download
        // Manager; cookies/session ride the same request context, so auth carries over.
        DownloadService downloads = getViewStack().getMainWindow() != null
            ? getViewStack().getMainWindow().getServiceManager().getService(DownloadService.class) : null;
        if (downloads != null) client.addDownloadHandler(downloads.cefDownloadHandler());

        browser = client.createBrowser(url, false, false);
        getComponent().remove(status);

        toolbar = new JToolBar();
        toolbar.setFloatable(false);
        toolbar.setOpaque(true);
        toolbar.setBackground(ThemeManager.getTintColor());

        if (supportsBookmarks()) {
            bookmarkBtn = new JButton("☆");
            bookmarkBtn.setToolTipText("Bookmark this site");
            bookmarkBtn.addActionListener(e -> toggleBookmark());
            toolbar.add(bookmarkBtn);
        }

        uriField = new JTextField(currentUri != null ? currentUri : "");
        uriField.addActionListener(e -> getViewStack().navigate(uriField.getText().trim()));
        toolbar.add(uriField);

        btnRefresh = new JButton("⟳");
        btnRefresh.setToolTipText("Reload");
        btnRefresh.addActionListener(e -> { if (browser != null) browser.reload(); });
        toolbar.add(btnRefresh);

        getComponent().add(toolbar, BorderLayout.NORTH);
        getComponent().add(browser.getUIComponent(), BorderLayout.CENTER);
        revalidate();
        repaint();
        updateStar();
        getViewStack().refreshNavState();
    }

    // ── Bookmarking ───────────────────────────────────────────────────────────────

    private void toggleBookmark() {
        if (currentUri == null) return;
        String uri = currentUri, title = currentTitle;
        bookmarkBtn.setEnabled(false);
        new SwingWorker<Void, Void>() {
            @Override protected Void doInBackground() {   // DB + favicon network I/O
                BookmarkManager.toggle(uri, title);
                return null;
            }
            @Override protected void done() {
                bookmarkBtn.setEnabled(true);
                updateStar();
                BookmarkEvents.fireChanged();
            }
        }.execute();
    }

    private void updateStar() {
        if (bookmarkBtn == null) return;
        bookmarkBtn.setText(currentUri != null && BookmarkManager.isBookmarked(currentUri) ? "★" : "☆");
    }

    private void updateColors() {
        Color bg = ThemeManager.getBackground();
        getComponent().setBackground(bg);
        status.setForeground(ThemeManager.getForeground());
        repaint();
    }
}
