package se.bungalow.content;

import se.bungalow.app.BungalowApp;
import se.bungalow.ui.MainWindow;

public class Context {
    private BungalowApp application;
    public BungalowApp getApplication() {
        return application;
    }
    public void setApplication(BungalowApp application) {
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
