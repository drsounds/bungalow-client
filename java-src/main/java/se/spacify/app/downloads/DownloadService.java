package se.spacify.app.downloads;

import org.cef.browser.CefBrowser;
import org.cef.callback.CefBeforeDownloadCallback;
import org.cef.callback.CefDownloadItem;
import org.cef.callback.CefDownloadItemCallback;
import org.cef.handler.CefDownloadHandler;
import org.cef.handler.CefDownloadHandlerAdapter;

import javax.swing.SwingUtilities;
import javax.swing.Timer;

import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;
import se.spacify.broadcast.Broadcast;
import se.spacify.broadcast.BroadcastManager;
import se.spacify.db.DatabaseManager;
import se.spacify.app.downloads.model.Download;
import se.spacify.service.Service;
import se.spacify.ui.MainWindow;
import se.spacify.web.CefRuntime;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Captures audio downloads from the in-app store/web views and routes them to a
 * matching {@link DownloadCaptureScope}. Cookies/session are preserved because
 * downloads ride the same global {@code CefRequestContext} as the browser — so
 * paid downloads work. Progress and completion are persisted to a {@link Download}
 * row and announced over the {@link BroadcastManager}.
 */
public final class DownloadService implements Service {

    /** Per-download runtime state (not persisted). */
    private static final class Job {
        final Download row;
        final DownloadCaptureScope scope;
        final PendingDownload pending;
        CefDownloadItemCallback callback;   // latest, for cancel
        @SuppressWarnings("unused")
        long speed;                          // bytes/sec, live
        int  lastPercent = -2;               // throttling: last persisted/announced percent
        long lastUpdate;                     // throttling: last persist time
        Job(Download row, DownloadCaptureScope scope, PendingDownload pending) {
            this.row = row; this.scope = scope; this.pending = pending;
        }
    }

    /** A hidden browser created to re-trigger a retry; disposed when its download ends. */
    private record RetryHandle(org.cef.CefClient client, CefBrowser browser) {}

    // CEF download-item id → job (active transfers)
    private final Map<Integer, Job> jobsByItem = new ConcurrentHashMap<>();
    // Download row id → live speed, for the view's Speed column
    private final Map<Integer, Long> speeds = new ConcurrentHashMap<>();
    // URLs queued for retry → the existing row id to reuse
    private final Map<String, Integer> retryRowByUrl = new ConcurrentHashMap<>();
    // Download row id → the hidden retry browser to dispose once it finishes
    private final Map<Integer, RetryHandle> retryByRow = new ConcurrentHashMap<>();

    private final CefDownloadHandler handler = new CefDownloadHandlerAdapter() {
        @Override
        public boolean onBeforeDownload(CefBrowser browser, CefDownloadItem item,
                String suggestedName, CefBeforeDownloadCallback callback) {
            PendingDownload pending = new PendingDownload(
                item.getURL(), suggestedName, item.getMimeType(), item.getTotalBytes());

            DownloadCaptureScope scope = findScope(pending);
            if (scope == null) return false;   // not an audio (or otherwise claimed) download → ignore

            Download row = reuseOrCreateRow(pending, scope);
            File target = new File(row.getFilePath());
            jobsByItem.put(item.getId(), new Job(row, scope, pending));
            callback.Continue(target.getAbsolutePath(), false);   // managed path, no Save-As dialog
            broadcastChanged();
            return true;
        }

        @Override
        public void onDownloadUpdated(CefBrowser browser, CefDownloadItem item,
                CefDownloadItemCallback callback) {
            Job job = jobsByItem.get(item.getId());
            if (job == null) return;
            job.callback = callback;
            job.speed = item.getCurrentSpeed();
            speeds.put(job.row.getId(), Math.max(0, item.getCurrentSpeed()));

            Download row = job.row;
            row.setReceivedBytes(item.getReceivedBytes());
            if (item.getTotalBytes() > 0) row.setTotalBytes(item.getTotalBytes());

            if (item.isComplete()) {
                row.setStatus(Download.Status.COMPLETE);
                row.setCompletedAt(System.currentTimeMillis());
                save(row);
                finish(item.getId(), job);
                importAsync(job);
            } else if (item.isCanceled()) {
                row.setStatus(Download.Status.CANCELLED);
                save(row);
                finish(item.getId(), job);
                broadcastChanged();
            } else {
                row.setStatus(Download.Status.DOWNLOADING);
                // onDownloadUpdated fires many times a second; only persist/announce
                // when the percent changes or ~300ms have passed, to avoid hammering
                // SQLite and reloading the table on every tick.
                int pct = row.percent();
                long now = System.currentTimeMillis();
                if (pct != job.lastPercent || now - job.lastUpdate >= 300) {
                    job.lastPercent = pct;
                    job.lastUpdate = now;
                    save(row);
                    broadcastChanged();
                }
            }
        }
    };

    /** The handler the web views attach to their CefClient. */
    public CefDownloadHandler cefDownloadHandler() { return handler; }

    /** Live download speed (bytes/sec) for a row, or 0 if not actively downloading. */
    public long speedFor(int rowId) { return speeds.getOrDefault(rowId, 0L); }

    /** Cancel an in-progress download (keeps the row for retry). */
    public void cancel(int rowId) {
        for (Job job : jobsByItem.values()) {
            if (job.row.getId() == rowId && job.callback != null) { job.callback.cancel(); return; }
        }
    }

    /**
     * Re-attempt a finished/failed download by re-loading its URL in a hidden
     * browser that shares the session cookies. Best-effort: only works while the
     * store's download link is still valid (many paid links expire).
     */
    public void retry(int rowId) {
        Download row = byId(rowId);
        if (row == null) return;
        row.setStatus(Download.Status.QUEUED);
        save(row);
        retryRowByUrl.put(row.getUrl(), row.getId());
        broadcastChanged();
        try {
            org.cef.CefClient client = CefRuntime.newClient();
            client.addDownloadHandler(handler);
            CefBrowser browser = client.createBrowser(row.getUrl(), false, false);   // triggers onBeforeDownload again
            retryByRow.put(row.getId(), new RetryHandle(client, browser));
            // If the link is dead (page loads, no download starts), tear the hidden
            // browser down anyway after a grace period so it doesn't leak.
            int id = row.getId();
            Timer timeout = new Timer(60_000, e -> disposeRetry(retryByRow.remove(id)));
            timeout.setRepeats(false);
            timeout.start();
        } catch (Exception e) {
            retryRowByUrl.remove(row.getUrl());
            row.setStatus(Download.Status.FAILED);
            save(row);
            broadcastChanged();
        }
    }

    /** Close a hidden retry browser and dispose its client (no-op if already gone). */
    private void disposeRetry(RetryHandle handle) {
        if (handle == null) return;
        SwingUtilities.invokeLater(() -> {
            try { handle.browser().close(true); } catch (Exception ignored) {}
            try { handle.client().dispose(); }    catch (Exception ignored) {}
        });
    }

    // ── Scope routing ───────────────────────────────────────────────────────────

    private DownloadCaptureScope findScope(PendingDownload pending) {
        MainWindow mw = MainWindow.getInstance();
        if (mw == null) return null;
        for (DownloadCaptureScope scope : mw.getServiceManager().getServices(DownloadCaptureScope.class)) {
            if (scope.accepts(pending)) return scope;
        }
        return null;
    }

    private void importAsync(Job job) {
        new Thread(() -> {
            try {
                job.scope.onCompleted(new File(job.row.getFilePath()), job.pending, job.row);
                save(job.row);
            } catch (Exception e) {
                job.row.setStatus(Download.Status.FAILED);
                save(job.row);
            }
            BroadcastManager.get().send(Broadcast.of(DownloadActions.COMPLETED).with("id", job.row.getId()));
            broadcastChanged();
        }, "download-import-" + job.row.getId()).start();
    }

    // ── Rows / persistence ────────────────────────────────────────────────────────

    private Download reuseOrCreateRow(PendingDownload pending, DownloadCaptureScope scope) {
        Integer retryId = retryRowByUrl.remove(pending.url());
        Download row = retryId != null ? byId(retryId) : null;
        if (row == null) {
            File target = uniqueTarget(pending.suggestedName());
            row = new Download(pending.url(), target.getName(), target.getAbsolutePath(),
                    pending.mimeType(), scope.scopeId());
        }
        row.setStatus(Download.Status.DOWNLOADING);
        row.setTotalBytes(pending.totalBytes());
        save(row);
        return row;
    }

    private void save(Download row) {
        try { DatabaseManager.getInstance().dao(Download.class).createOrUpdate(row); }
        catch (Exception e) { System.err.println("Download persist failed: " + e); }
    }

    private Download byId(int id) {
        try { return DatabaseManager.getInstance().dao(Download.class).queryForId(id); }
        catch (Exception e) { return null; }
    }

    private void finish(int itemId, Job job) {
        jobsByItem.remove(itemId);
        speeds.remove(job.row.getId());
        disposeRetry(retryByRow.remove(job.row.getId()));
    }

    private void broadcastChanged() {
        BroadcastManager.get().send(Broadcast.of(DownloadActions.CHANGED).with("active", jobsByItem.size()));
    }

    // ── Managed download folder ────────────────────────────────────────────────────

    /** The managed downloads folder: {@code download.dir} from settings, else ~/.spacify/downloads. */
    public static File downloadsDir() {
        String configured = null;
        Path cfg = Path.of(System.getProperty("user.home"), ".spacify", "settings.properties");
        if (Files.exists(cfg)) {
            Properties p = new Properties();
            try (InputStream in = Files.newInputStream(cfg)) { p.load(in); configured = p.getProperty("download.dir"); }
            catch (IOException ignored) {}
        }
        File dir = (configured != null && !configured.isBlank())
            ? new File(configured)
            : new File(System.getProperty("user.home"), ".spacify/downloads");
        dir.mkdirs();
        return dir;
    }

    private static File uniqueTarget(String suggestedName) {
        String name = (suggestedName == null || suggestedName.isBlank()) ? "download" : suggestedName;
        File dir = downloadsDir();
        File target = new File(dir, name);
        if (!target.exists()) return target;
        String base = name, ext = "";
        int dot = name.lastIndexOf('.');
        if (dot > 0) { base = name.substring(0, dot); ext = name.substring(dot); }
        for (int i = 1; ; i++) {
            File candidate = new File(dir, base + " (" + i + ")" + ext);
            if (!candidate.exists()) return candidate;
        }
    }

    // ── Service ─────────────────────────────────────────────────────────────────

    @Override public String getId()   { return "downloads"; }
    @Override public String getName() { return "Download Manager"; }
    @Override public void onRegister(AspectManager<? extends Aspect> manager) {}

    @Override
    public void onStart() {
        // A download in progress can't survive a restart — mark interrupted ones failed.
        try {
            DatabaseManager db = DatabaseManager.getInstance();
            List<Download> active = db.dao(Download.class).queryForEq("status", Download.Status.DOWNLOADING.name());
            for (Download d : active) { d.setStatus(Download.Status.FAILED); db.dao(Download.class).update(d); }
        } catch (Exception ignored) {}
    }
}
