package se.bungalow.navigation;

public interface NavigationListener {
    void onNavigate(String uri, boolean canGoBack, boolean canGoForward);
}
