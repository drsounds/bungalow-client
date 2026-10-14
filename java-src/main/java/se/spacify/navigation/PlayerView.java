package se.spacify.navigation;

import javax.swing.JComponent;

public abstract class PlayerView {

    public abstract String getName();

    public abstract JComponent getComponent();

    public void onTrackChanged(String title, String artist, String album) {}

    public void onShow() {}

    public void onHide() {}
}
