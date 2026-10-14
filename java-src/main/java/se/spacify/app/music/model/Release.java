package se.spacify.app.music.model;

/**
 * Abstract, generic base for a published release of some content type — the
 * music {@link MusicRelease} today, with {@code VideoRelease}, {@code GameRelease}
 * etc. to follow. Being {@link Content} itself, a release shares the credit and
 * commerce infrastructure with the items it collects.
 *
 * @param <T> the content type this release is composed of (e.g. {@link Recording})
 */
public abstract class Release<T extends Content<?>> extends Content<ReleaseCreatorCredit> {
}
