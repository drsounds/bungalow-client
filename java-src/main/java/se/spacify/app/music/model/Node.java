package se.spacify.app.music.model;

import com.j256.ormlite.field.DatabaseField;

/**
 * Abstract root of the catalogue model. Every persistable domain object
 * ({@link Content}, {@link Creator}, {@link ContentCollection}…) is a Node and so
 * exposes a human-readable {@link #getName() name} and a {@link #getVersion()
 * version}. Sharing this base lets views and services treat heterogeneous
 * content uniformly.
 */
public abstract class Node {

    @DatabaseField(canBeNull = false)
    private String name;

    /** Monotonic revision counter, bumped on edits; 0 for freshly created rows. */
    @DatabaseField
    private int version;

    public String getName()            { return name; }
    public void   setName(String v)    { this.name = v; }
    public int    getVersion()         { return version; }
    public void   setVersion(int v)    { this.version = v; }

    /** @deprecated legacy alias for {@link #getName()} — "name" is the standard now. */
    @Deprecated public String getTitle()           { return name; }
    /** @deprecated legacy alias for {@link #setName(String)}. */
    @Deprecated public void   setTitle(String v)   { this.name = v; }
}
