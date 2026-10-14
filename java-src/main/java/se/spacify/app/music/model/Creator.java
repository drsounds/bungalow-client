package se.spacify.app.music.model;

import com.j256.ormlite.field.DatabaseField;

/**
 * Abstract base for anything that can be credited on {@link Content}: musicians,
 * authors, studios, developers… Music's {@link Artist} is the first concrete
 * Creator; future verticals (e.g. a video {@code Director} or game {@code Studio})
 * subclass this too so the credit infrastructure ({@link CreatorCredit}) is shared.
 */
public abstract class Creator extends Node {

    @DatabaseField(generatedId = true)
    private int id;

    public int getId() { return id; }
}
