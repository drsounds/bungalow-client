package se.spacify.app.music.model;

import com.j256.ormlite.dao.ForeignCollection;
import com.j256.ormlite.field.DatabaseField;

/**
 * Abstract base for any creditable, displayable piece of media — a music
 * {@link Recording} or {@link Release} today, a {@code Video}, {@code Episode} or
 * {@code Game} tomorrow. Carries the shared creator-credit collection so the same
 * UI (the "Buy", "Play with…" affordances, credit lists) works across verticals.
 *
 * @param <C> the concrete {@link CreatorCredit} type backing this content
 */
public abstract class Content<C extends CreatorCredit<?>> extends Node {

    @DatabaseField(generatedId = true)
    private int id;

    /**
     * Display-only creator credit for transient (catalogue / discovery) content
     * that isn't backed by DB {@link CreatorCredit} rows. Not a
     * {@code @DatabaseField}, so ORMLite ignores it; persisted content carries its
     * creators in {@link #getCreatorCredits()} instead.
     */
    private String artistNames;

    public int getId() { return id; }

    /** The persisted creator credits for this content. */
    public abstract ForeignCollection<C> getCreatorCredits();

    public abstract void setCreatorCredits(ForeignCollection<C> credits);

    public String getArtistNames()         { return artistNames; }
    public void   setArtistNames(String v) { this.artistNames = v; }
}
