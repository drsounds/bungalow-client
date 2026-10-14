package se.spacify.app.media.views;

import se.spacify.controls.ToolBar;
import se.spacify.navigation.PlayerView;
import se.spacify.navigation.View;
import se.spacify.navigation.ViewStack;
import se.spacify.app.media.service.MediaService;
import se.spacify.ui.SettingsPanel;
import se.spacify.ui.theme.ThemeManager;

import javax.swing.*;
import java.awt.*;
import java.util.ArrayList;
import java.util.List;

public class NowPlayingView extends View {

    private final JPanel playerContainer;
    private final JComboBox<String> viewSelector;
    private final List<PlayerView> playerViews = new ArrayList<>();
    private PlayerView activeView;
	private ToolBar topToolbar;

    public NowPlayingView(ViewStack viewStack) {
        super(viewStack);
        getComponent().setLayout(new BorderLayout());
        getComponent().setOpaque(true);
        getComponent().setBackground(Color.BLACK);

        topToolbar = new ToolBar();
        topToolbar.getComponent().setFloatable(false);
        topToolbar.getComponent().setOpaque(true);
        topToolbar.getComponent().setBackground(ThemeManager.getTintColor());
        add(topToolbar, BorderLayout.NORTH);
        topToolbar.getComponent().add(new JButton("<"));
        // ── Player area ──────────────────────────────────────────────────────
        playerContainer = new JPanel(new BorderLayout());
        playerContainer.setOpaque(false);

        // Selector bar shown only when more than one PlayerView is registered
        viewSelector = new JComboBox<>();
        viewSelector.setVisible(false);
        viewSelector.addActionListener(e -> {
            int idx = viewSelector.getSelectedIndex();
            if (idx >= 0 && idx < playerViews.size()) {
                activateView(playerViews.get(idx));
            }
        });

        JPanel selectorBar = new JPanel(new FlowLayout(FlowLayout.RIGHT, 8, 4));
        selectorBar.setOpaque(false);
        selectorBar.add(new JLabel("View:"));
        selectorBar.add(viewSelector);

        JPanel playerWrapper = new JPanel(new BorderLayout());
        playerWrapper.setOpaque(false);
        playerWrapper.add(selectorBar,    BorderLayout.NORTH);
        playerWrapper.add(playerContainer, BorderLayout.CENTER);

        // ── Settings strip ───────────────────────────────────────────────────
        SettingsPanel settings = new SettingsPanel();

        getComponent().add(playerWrapper, BorderLayout.CENTER);
        getComponent().add(settings.getComponent(), BorderLayout.SOUTH);

        // Default view
        addPlayerView(new DefaultPlayerView());
    }

    public void addPlayerView(PlayerView view) {
        playerViews.add(view);
        viewSelector.addItem(view.getName());
        viewSelector.setVisible(playerViews.size() > 1);
        if (activeView == null) activateView(view);
    }

    private void activateView(PlayerView view) {
        if (activeView != null) {
            activeView.onHide();
            playerContainer.remove(activeView.getComponent());
        }
        activeView = view;
        playerContainer.add(view.getComponent(), BorderLayout.CENTER);
        playerContainer.revalidate();
        playerContainer.repaint();
        view.onShow();
    }

    @Override
    public boolean acceptsUri(String uri) {
        return uri != null && uri.matches("spacify:(home|now-playing)");
    }

    @Override
    public void navigate(String uri) {}


    /** Wire a MediaService so track-change events propagate to the active PlayerView. */
    public void setMediaService(MediaService ms) {
        ms.addPlaybackListener(new MediaService.PlaybackListener() {
            @Override public void onStateChanged(MediaService.PlaybackState s) {}
            @Override public void onPositionChanged(long pos, long dur) {}
            @Override public void onError(Exception e) {}
            @Override
            public void onTrackChanged(String title, String artist, String album) {
                if (activeView != null) activeView.onTrackChanged(title, artist, album);
            }
        });
    }

    @Override
    public String getName() { return "Now Playing"; }
}
