package se.spacify.app.music.model;

import com.j256.ormlite.field.DatabaseField;

/**
 * Abstract base for a single positioned entry in a {@link ContentCollection}.
 * A {@link Track} (a recording's place on a release) and a {@link PlaylistRow}
 * (a content URI's place in a playlist) are both collection rows.
 *
 * @param <T> the content type this row points at
 */
public abstract class ContentCollectionRow<T extends Content<?>> {

    @DatabaseField(generatedId = true)
    private int id;

    /** Zero-based position within the owning collection. ("index" is reserved SQL.) */
    @DatabaseField(columnName = "row_index")
    private int position;

    public int  getId()             { return id; }
    public int  getPosition()       { return position; }
    public void setPosition(int v)  { this.position = v; }
}
