package se.spacify.web;

import com.j256.ormlite.dao.Dao;
import se.spacify.db.DatabaseManager;
import se.spacify.app.web.model.Bookmark;

import java.util.List;

/**
 * Toggles and queries hierarchical site bookmarks.
 *
 * <p>A root bookmark (parent == null) represents a host; sub-pages are children
 * of it. {@code autoCreated} marks a root that exists only as a container for
 * children (the index page itself is not bookmarked). The "is this page
 * bookmarked?" question is therefore:
 * <ul>
 *   <li>root page → its root bookmark exists and is not auto-created</li>
 *   <li>sub-page  → a child bookmark for that exact URI exists</li>
 * </ul>
 *
 * <p>DB plus favicon network I/O — call {@link #toggle} off the EDT.
 */
public final class BookmarkManager {

    private BookmarkManager() {}

    private static Dao<Bookmark, Integer> dao() {
        return DatabaseManager.getInstance().dao(Bookmark.class);
    }

    private static Bookmark find(String spacifyUri) {
        try {
            return dao().queryForEq("spacify_uri", spacifyUri).stream().findFirst().orElse(null);
        } catch (Exception e) {
            return null;
        }
    }

    /** Whether the given page is currently bookmarked (for the star state). */
    public static boolean isBookmarked(String spacifyUri) {
        if (!SiteUri.isSiteUri(spacifyUri)) return false;
        if (SiteUri.isRoot(spacifyUri)) {
            Bookmark root = find(SiteUri.rootUri(spacifyUri));
            return root != null && !root.isAutoCreated();
        }
        return find(spacifyUri) != null;
    }

    /** Add the bookmark if absent, otherwise remove it. */
    public static void toggle(String spacifyUri, String pageTitle) {
        if (!SiteUri.isSiteUri(spacifyUri)) return;
        if (isBookmarked(spacifyUri)) remove(spacifyUri);
        else                          add(spacifyUri, pageTitle);
    }

    // ── add ─────────────────────────────────────────────────────────────────────

    private static void add(String spacifyUri, String pageTitle) {
        String host    = SiteUri.host(spacifyUri);
        String rootUri = SiteUri.rootUri(spacifyUri);
        try {
            if (SiteUri.isRoot(spacifyUri)) {
                Bookmark root = find(rootUri);
                if (root == null) {
                    root = new Bookmark(host, rootUri, label(pageTitle, host));
                    root.setFavicon(FaviconFetcher.fetch(host));
                    dao().create(root);
                } else {                       // promote a container to an explicit bookmark
                    root.setAutoCreated(false);
                    if (pageTitle != null && !pageTitle.isBlank()) root.setName(pageTitle);
                    dao().update(root);
                }
            } else {
                Bookmark root = ensureRoot(host, rootUri);
                if (find(spacifyUri) == null) {
                    Bookmark child = new Bookmark(host, spacifyUri, label(pageTitle, pathOf(spacifyUri, host)));
                    child.setParent(root);
                    child.setFavicon(root.getFavicon());
                    dao().create(child);
                }
            }
        } catch (Exception ignored) {}
    }

    /** Find or create the (possibly auto-created) container root for a host. */
    private static Bookmark ensureRoot(String host, String rootUri) throws Exception {
        Bookmark root = find(rootUri);
        if (root == null) {
            root = new Bookmark(host, rootUri, host);
            root.setAutoCreated(true);
            root.setFavicon(FaviconFetcher.fetch(host));
            dao().create(root);
        }
        return root;
    }

    // ── remove ───────────────────────────────────────────────────────────────────

    private static void remove(String spacifyUri) {
        try {
            if (SiteUri.isRoot(spacifyUri)) {
                Bookmark root = find(SiteUri.rootUri(spacifyUri));
                if (root == null) return;
                if (childCount(root) > 0) {     // keep as a container for its children
                    root.setAutoCreated(true);
                    dao().update(root);
                } else {
                    dao().delete(root);
                }
            } else {
                Bookmark child = find(spacifyUri);
                if (child == null) return;
                Bookmark root = child.getParent();
                dao().delete(child);
                if (root != null && root.isAutoCreated() && childCount(root) == 0) {
                    dao().delete(root);
                }
            }
        } catch (Exception ignored) {}
    }

    private static int childCount(Bookmark root) throws Exception {
        return dao().queryForEq("parent_id", root.getId()).size();
    }

    /** Top-level (root) bookmarks, for sidebar population. */
    public static List<Bookmark> roots() {
        try {
            return dao().queryBuilder().where().isNull("parent_id").query();
        } catch (Exception e) {
            return List.of();
        }
    }

    public static List<Bookmark> childrenOf(Bookmark root) {
        try {
            return dao().queryForEq("parent_id", root.getId());
        } catch (Exception e) {
            return List.of();
        }
    }

    // ── helpers ───────────────────────────────────────────────────────────────────

    private static String label(String title, String fallback) {
        return (title != null && !title.isBlank()) ? title.trim() : fallback;
    }

    private static String pathOf(String spacifyUri, String host) {
        String prefix = SiteUri.PREFIX + host;
        String rest = spacifyUri.startsWith(prefix) ? spacifyUri.substring(prefix.length()) : spacifyUri;
        return rest.isBlank() ? host : rest.replaceFirst("^:", "");
    }
}
