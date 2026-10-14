package se.spacify.app.music.model;

import com.j256.ormlite.field.DatabaseField;

/**
 * Abstract base for an ordered collection of {@link Content} — the foundation for
 * {@link Playlist} and any future curated list. Subclasses expose their rows as
 * {@link ContentCollectionRow}s.
 *
 * @param <T> the content type this collection holds
 */
public abstract class ContentCollection<T extends Content<?>> extends Node {

    @DatabaseField(generatedId = true)
    private int id;

    public int getId() { return id; }
}
