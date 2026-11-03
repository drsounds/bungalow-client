package se.bungalow.app.playlist.views;

import se.bungalow.app.music.controls.MusicTable;
import se.bungalow.app.music.net.MusikUri;
import se.bungalow.app.music.views.AbstractMusicListView;
import se.bungalow.app.playlist.model.Playlist;
import se.bungalow.app.playlist.service.PlaylistItem;
import se.bungalow.app.playlist.service.PlaylistService;
import se.bungalow.app.playlist.upsl.Upsl;
import se.bungalow.controls.ToolBar;
import se.bungalow.controls.ToolButton;
import se.bungalow.navigation.ViewStack;
import se.bungalow.service.media.PlayRequest;

import javax.swing.*;
import java.util.ArrayList;
import java.util.List;

/**
 * Shows a decentralized (federated) playlist read straight from its UPSL link
 * (RFC-0001) — {@code musik:federated:user:<handle>:playlist:<uuid>:<body>}. The
 * link is self-describing, so no host needs to be reachable: the name, issuer
 * handle and the {@code musik:} content refs are decoded locally and listed in a
 * {@link MusicTable}. Rows play through the normal resolver (each recipient plays
 * on their own services) and can be dragged into local playlists; "Save as
 * playlist" copies the whole list into the first editable {@link PlaylistService}.
 * A hosted link (no body) has nothing to show until a host fetch is implemented.
 */
public class FederatedPlaylistView extends AbstractMusicListView {

    private Upsl upsl;
    private final List<PlaylistItem> items = new ArrayList<>();

    public FederatedPlaylistView(ViewStack viewStack) {
        super(viewStack);
    }

    @Override protected boolean isEditable() { return false; }

    @Override protected List<MusicTable.Column> getColumns() {
        return List.of(
            column("number",   "#"),
            column("title",    "Title"),
            column("artist",   "Artist"),
            column("bungalow:buy-stream", "Buy/Stream")
        );
    }

    @Override
    protected JComponent toolbarAccessory() {
        ToolBar bar = new ToolBar();
        ToolButton save = new ToolButton("Save as playlist");
        save.getComponent().addActionListener(e -> saveAsPlaylist());
        bar.add(save);
        return bar.getComponent();
    }

    // ── Navigation ──────────────────────────────────────────────────────────────

    @Override public boolean acceptsUri(String uri) {
        return Upsl.isUpsl(uri) && !uri.startsWith("acct:");
    }

    @Override
    public void navigate(String uri) {
        try {
            upsl = Upsl.parse(uri);
        } catch (IllegalArgumentException e) {
            upsl = null;   // a leaf that doesn't cleanly decode is rejected (RFC-0001 §12)
        }
        reloadAndRegroup();
    }

    @Override public String getName() { return "Federated playlist"; }

    // ── Rendering ───────────────────────────────────────────────────────────────

    @Override
    protected void reload() {
        items.clear();
        musicTable.clear();
        if (upsl == null) { setHeader("Invalid playlist link"); return; }
        String name = upsl.name().isEmpty() ? "Shared playlist" : upsl.name();
        setHeader(name + " — shared by " + upsl.handle());
        int n = 1;
        for (String ref : upsl.refs()) {
            PlaylistItem item = toItem(ref);
            items.add(item);
            addRow(row()
                .set("number", n++)
                .set("title",  item.getName())
                .set("artist", item.getArtist() == null ? "" : item.getArtist()));
        }
    }

    /** Name a content ref for display; non-recording refs keep the raw URI as their title. */
    private static PlaylistItem toItem(String ref) {
        MusikUri m = MusikUri.parse(ref);
        if (m != null && m.kind() == MusikUri.Kind.RECORDING) {
            return new PlaylistItem(ref, m.recording(), m.artist(), 0);
        }
        return new PlaylistItem(ref, ref, null, 0);
    }

    @Override
    protected PlayRequest playRequestAt(int row) {
        if (row < 0 || row >= items.size()) return null;
        PlaylistItem it = items.get(row);
        PlayRequest req = MusikUri.toPlayRequest(it.getPlayUri());
        if (req == null) return null;   // http(s) and non-track refs aren't resolver-playable
        String title = req.title() == null || req.title().isBlank() ? it.getName() : req.title();
        String artist = req.artist() == null || req.artist().isBlank()
            ? (it.getArtist() == null ? "" : it.getArtist()) : req.artist();
        return new PlayRequest(null, req.isrc(), title, artist, it.getPlayUri(), it.getDurationMs());
    }

    // ── Actions ─────────────────────────────────────────────────────────────────

    private void saveAsPlaylist() {
        if (upsl == null || items.isEmpty()) return;
        PlaylistService svc = null;
        for (PlaylistService s : getViewStack().getMainWindow().getServiceManager()
                .getServices(PlaylistService.class)) {
            if (s.isEditable()) { svc = s; break; }
        }
        if (svc == null) { showError(new Exception("No editable playlist service available")); return; }
        try {
            Playlist pl = svc.createPlaylist(upsl.name().isEmpty() ? "Shared playlist" : upsl.name());
            for (PlaylistItem item : items) svc.addToPlaylist(pl.getPublicId(), item);
            getViewStack().navigate("bungalow:playlist:" + pl.getPublicId());
        } catch (Exception e) {
            showError(e);
        }
    }
}
