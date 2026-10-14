package se.spacify.search;

/**
 * The domain abstraction a {@link SearchResult} represents. Drives the section a
 * result is grouped under in the search view, and the section's ordering.
 */
public enum EntityKind {
    ARTIST   ("Artists",  1),
    RELEASE  ("Releases", 2),
    RECORDING("Songs",    3),
    TRACK    ("Tracks",   4),
    VIDEO    ("Videos",   5),
    OTHER    ("Other",   99);

    private final String label;
    private final int    order;

    EntityKind(String label, int order) { this.label = label; this.order = order; }

    /** Section heading shown in the search view. */
    public String label() { return label; }
    /** Sort key for ordering sections. */
    public int order() { return order; }
}
