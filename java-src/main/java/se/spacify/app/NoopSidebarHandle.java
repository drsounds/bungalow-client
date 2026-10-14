package se.spacify.app;

import se.spacify.navigation.SidebarNode;

import java.util.List;

/** No-op handle returned when there is no live sidebar (e.g. headless tests). */
final class NoopSidebarHandle implements SidebarHandle {

    static final NoopSidebarHandle INSTANCE = new NoopSidebarHandle();

    private NoopSidebarHandle() {}

    @Override public SidebarHandle child(String uri) { return this; }
    @Override public void setChildren(List<SidebarNode> children) {}
    @Override public void expand() {}
}
