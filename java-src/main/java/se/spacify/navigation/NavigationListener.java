package se.spacify.navigation;

public interface NavigationListener {
    void onNavigate(String uri, boolean canGoBack, boolean canGoForward);
}
