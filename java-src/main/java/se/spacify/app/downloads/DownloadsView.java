package se.spacify.app.downloads;
import se.spacify.app.music.controls.MusicTable;
import se.spacify.app.music.views.AbstractMusicListView;

import se.spacify.broadcast.BroadcastManager;
import se.spacify.db.DatabaseManager;
import se.spacify.app.downloads.model.Download;
import se.spacify.navigation.ViewStack;
import se.spacify.service.media.PlayRequest;
import se.spacify.controls.Button;
import javax.swing.*;
import java.awt.*;
import java.io.File;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

/**
 * The Download Manager screen (spacify:downloads): an iTunes/Kazaa-style list of
 * downloads with progress, size and speed, plus Cancel / Retry / Reveal / Play.
 * Built on {@link AbstractMusicListView} so it inherits the library look (themed
 * 24px rows, full-height stripes). Updated live via the broadcast bus.
 */
public class DownloadsView extends AbstractMusicListView {

    private final List<Download> rows = new ArrayList<>();

    public DownloadsView(ViewStack viewStack) {
        super(viewStack);
        setHeader("Downloads");
        // Refresh whenever the download manager reports a change (any plugin/thread).
        BroadcastManager.get().register(DownloadActions.CHANGED, b -> reloadAndRegroup());
    }

    @Override protected boolean isEditable() { return false; }

    @Override protected List<MusicTable.Column> getColumns() {
        return List.of(
            column("name",     "Name"),
            column("artist",   "Artist"),
            column("album",    "Album"),
            column("size",     "Size"),
            column("progress", "Progress"),
            column("speed",    "Speed"),
            column("status",   "Status"));
    }

    @Override
    protected void reload() {
        rows.clear();
        musicTable.clear();
        try {
            List<Download> all = DatabaseManager.getInstance().dao(Download.class).queryForAll();
            all.sort(Comparator.comparingLong(Download::getCreatedAt).reversed());
            DownloadService svc = service();
            for (Download d : all) {
                rows.add(d);
                long speed = svc != null ? svc.speedFor(d.getId()) : 0L;
                addRow(row()
                    .set("name",     d.getName() != null ? d.getName() : d.getFileName())
                    .set("artist",   d.getArtist() != null ? d.getArtist() : "")
                    .set("album",    d.getAlbum() != null ? d.getAlbum() : "")
                    .set("size",     humanBytes(d.getTotalBytes() > 0 ? d.getTotalBytes() : d.getReceivedBytes()))
                    .set("progress", d.getStatus() == Download.Status.DOWNLOADING && d.percent() >= 0 ? d.percent() + "%"
                        : d.getStatus() == Download.Status.COMPLETE ? "100%" : "—")
                    .set("speed",    speed > 0 ? humanBytes(speed) + "/s" : "")
                    .set("status",   statusText(d.getStatus())));
            }
        } catch (Exception e) {
            showError(e);
        }
    }

    /** Action bar (Cancel / Retry / Reveal / Play) shown in the toolbar. */
    @Override
    protected JComponent toolbarAccessory() {
        JPanel bar = new JPanel(new FlowLayout(FlowLayout.RIGHT, 4, 0));
        bar.setOpaque(false);
        bar.add(button("Cancel",  this::cancelSelected).getComponent());
        bar.add(button("Retry",   this::retrySelected).getComponent());
        bar.add(button("Reveal",  this::revealSelected).getComponent());
        bar.add(button("Play",    this::playSelected).getComponent());
        return bar;
    }

    @Override
    protected PlayRequest playRequestAt(int row) {
        Download d = rows.get(row);
        if (d.getStatus() != Download.Status.COMPLETE || d.getFilePath() == null) return null;
        String uri = "spacify:local:" + d.getFilePath();
        return new PlayRequest(null, null, d.getName() != null ? d.getName() : d.getFileName(),
                d.getArtist() != null ? d.getArtist() : "", uri, 0);
    }

    // ── Actions ───────────────────────────────────────────────────────────────────

    private void cancelSelected() {
        Download d = selected();
        if (d != null && service() != null) service().cancel(d.getId());
    }

    private void retrySelected() {
        Download d = selected();
        if (d == null || service() == null) return;
        if (d.getStatus() == Download.Status.COMPLETE) return;
        service().retry(d.getId());
    }

    private void revealSelected() {
        Download d = selected();
        if (d == null || d.getFilePath() == null) return;
        File file = new File(d.getFilePath());
        File dir = file.getParentFile();
        if (dir != null && dir.isDirectory() && Desktop.isDesktopSupported()) {
            try { Desktop.getDesktop().open(dir); } catch (Exception ignored) {}
        }
    }

    private void playSelected() {
        int row = table.getSelectedRow();
        if (row < 0) return;
        PlayRequest req = playRequestAt(row);
        if (req != null) se.spacify.service.media.PlaybackCoordinator.resolveAndPlay(req);
    }

    private Download selected() {
        int row = table.getSelectedRow();
        return (row >= 0 && row < rows.size()) ? rows.get(row) : null;
    }

    private DownloadService service() {
        var mw = getViewStack().getMainWindow();
        return mw != null ? mw.getServiceManager().getService(DownloadService.class) : null;
    }

    // ── Formatting ──────────────────────────────────────────────────────────────────

    private static Button button(String text, Runnable action) {
        Button b = new Button(text);
        b.getComponent().setFocusPainted(false);
        b.getComponent().addActionListener(e -> action.run());
        return b;
    }

    private static String statusText(Download.Status s) {
        return switch (s) {
            case QUEUED -> "Queued";
            case DOWNLOADING -> "Downloading…";
            case COMPLETE -> "Complete";
            case FAILED -> "Failed";
            case CANCELLED -> "Cancelled";
        };
    }

    private static String humanBytes(long bytes) {
        if (bytes <= 0) return "—";
        String[] units = {"B", "KB", "MB", "GB"};
        double v = bytes; int u = 0;
        while (v >= 1024 && u < units.length - 1) { v /= 1024; u++; }
        return (u == 0 ? (long) v + " " : String.format("%.1f ", v)) + units[u];
    }

    @Override public boolean acceptsUri(String uri) { return "spacify:downloads".equals(uri); }
    @Override public String getName() { return "Downloads"; }
}
