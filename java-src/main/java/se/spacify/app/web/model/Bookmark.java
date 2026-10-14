package se.spacify.app.web.model;

import com.j256.ormlite.dao.ForeignCollection;
import com.j256.ormlite.field.DataType;
import com.j256.ormlite.field.DatabaseField;
import com.j256.ormlite.field.ForeignCollectionField;
import com.j256.ormlite.table.DatabaseTable;

/**
 * A bookmarked web site/page, shown under the sidebar's "Sites" node.
 *
 * <p>Bookmarks are hierarchical: a root bookmark (parent == null) represents a
 * site (host), and its children are bookmarked sub-pages of that host.
 * {@link #autoCreated} marks a root that exists only as a container for
 * sub-page bookmarks (i.e. the index page itself is not bookmarked); such a
 * root is removed automatically once its last child is removed.
 */
@DatabaseTable(tableName = "bookmarks")
public class Bookmark {

    @DatabaseField(generatedId = true)
    private int id;

    @DatabaseField(canBeNull = false)
    private String host;

    /** Full spacify:site: URI this bookmark points at (unique). */
    @DatabaseField(unique = true, canBeNull = false, columnName = "spacify_uri")
    private String spacifyUri;

    @DatabaseField(canBeNull = true)
    private String title;

    /** Favicon as 16x16 PNG bytes; shared across a host's bookmarks. */
    @DatabaseField(dataType = DataType.BYTE_ARRAY, canBeNull = true)
    private byte[] favicon;

    /** True if this root exists only to hold children (index not bookmarked). */
    @DatabaseField(columnName = "auto_created")
    private boolean autoCreated;

    @DatabaseField(foreign = true, foreignAutoRefresh = true, canBeNull = true, columnName = "parent_id")
    private Bookmark parent;

    @ForeignCollectionField(eager = true)
    private ForeignCollection<Bookmark> children;

    public Bookmark() {}

    public Bookmark(String host, String spacifyUri, String title) {
        this.host = host;
        this.spacifyUri = spacifyUri;
        this.title = title;
    }

    public int      getId()                  { return id; }
    public String   getHost()                { return host; }
    public void     setHost(String v)        { this.host = v; }
    public String   getSpacifyUri()          { return spacifyUri; }
    public void     setSpacifyUri(String v)  { this.spacifyUri = v; }
    public String   getName()               { return title; }
    public void     setName(String v)       { this.title = v; }
    public byte[]   getFavicon()             { return favicon; }
    public void     setFavicon(byte[] v)     { this.favicon = v; }
    public boolean  isAutoCreated()          { return autoCreated; }
    public void     setAutoCreated(boolean v){ this.autoCreated = v; }
    public Bookmark getParent()              { return parent; }
    public void     setParent(Bookmark v)    { this.parent = v; }
    public ForeignCollection<Bookmark> getChildren() { return children; }

    @Override public String toString() { return title != null ? title : spacifyUri; }
}
