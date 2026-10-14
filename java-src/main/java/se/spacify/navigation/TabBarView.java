package se.spacify.navigation;

import se.spacify.controls.Control;
import se.spacify.controls.TabbedPane;

public class TabBarView extends View {
    private TabbedPane tabbedPane;

    public TabbedPane getTabbedPane() {
        return tabbedPane;
    }

    public TabBarView(ViewStack viewStack) {
        super(viewStack);        
        this.tabbedPane = new TabbedPane();
        add(this.tabbedPane);
        
    }

    @Override
    public boolean acceptsUri(String uri) {
        return uri != null && uri.indexOf('#') >= 0;
    }

    /**
     * When the URI carries a fragment ({@code …#section}), the part after '#' is
     * taken as a tab id and we switch to the tab whose control has that id. A bare
     * trailing '#' (empty section) leaves the current tab untouched.
     */
    @Override
    public void navigate(String uri) {
        if (uri == null) {
            return;
        }
        int hash = uri.indexOf('#');
        if (hash < 0) {
            return;
        }
        String section = uri.substring(hash + 1);
        if (section.isEmpty()) {
            return;
        }
        selectTabById(section);
    }

    /** Switch to the tab whose child control id matches {@code id}, if any. */
    private void selectTabById(String id) {
        var tabs = tabbedPane.getChildren();
        for (int i = 0; i < tabs.size(); i++) {
            if (id.equals(tabId(tabs.get(i)))) {
                tabbedPane.getComponent().setSelectedIndex(i);
                return;
            }
        }
    }

    /** A tab's id: its control {@link Control#getName() name}, falling back to the {@code id} attribute. */
    private static String tabId(Control<?> tab) {
        if (tab.getName() != null) {
            return tab.getName();
        }
        Object attr = tab.getAttribute("id", null);
        return attr != null ? attr.toString() : null;
    }
}
