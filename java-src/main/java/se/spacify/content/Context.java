package se.spacify.content;

import se.spacify.app.SpacifyApp;
import se.spacify.ui.MainWindow;

public class Context {
    private SpacifyApp application;
    public SpacifyApp getApplication() {
        return application;
    }
    public void setApplication(SpacifyApp application) {
        this.application = application;
    }

    private MainWindow window;
    public MainWindow getWindow() {
        return window;
    }
    public void setWindow(MainWindow window) {
        this.window = window;
    }
}
