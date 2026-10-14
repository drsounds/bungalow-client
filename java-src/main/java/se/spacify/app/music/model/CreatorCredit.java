package se.spacify.app.music.model;

import com.j256.ormlite.field.DatabaseField;

/**
 * Abstract join row crediting a {@link Creator} on a piece of {@link Content}.
 * Parameterised by the content type so concrete subclasses
 * ({@link RecordingCreatorCredit}, {@link ReleaseCreatorCredit}) bind to the
 * right table while sharing the common credit metadata (primary flag, role).
 *
 * @param <T> the content type this credit attaches to
 */
public abstract class CreatorCredit<T extends Content<?>> {

    @DatabaseField(generatedId = true)
    private int id;

    /** True if this is the primary/credited creator (vs. a featured one). */
    @DatabaseField
    private boolean primary;

    /** e.g. "performer", "composer", "producer", "conductor", "lyricist". */
    @DatabaseField(canBeNull = true)
    private String role;

    public int     getId()               { return id; }
    public boolean isPrimary()           { return primary; }
    public void    setPrimary(boolean v) { this.primary = v; }
    public String  getRole()             { return role; }
    public void    setRole(String v)     { this.role = v; }

    /** The credited creator (an {@link Artist} for music). */
    public abstract Creator getCreator();

    /** The content this credit is attached to. */
    public abstract T getContent();
}
