package se.bungalow.web;

import com.j256.ormlite.dao.Dao;
import se.bungalow.db.DatabaseManager;
import se.bungalow.app.web.model.Bookmark;

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

    private static Bookmark find(String bungalowUri) {
        try {
            return dao().queryForEq("bungalow_uri", bungalowUri).stream().findFirst().orElse(null);
        } catch (Exception e) {
            return null;
        }
    }

    /** Whether the given page is currently bookmarked (for the star state). */
    public static boolean isBookmarked(String bungalowUri) {
        if (!SiteUri.isSiteUri(bungalowUri)) return false;
        if (SiteUri.isRoot(bungalowUri)) {
            Bookmark root = find(SiteUri.rootUri(bungalowUri));
            return root != null && !root.isAutoCreated();
        }
        return find(bungalowUri) != null;
    }

    /** Add the bookmark if absent, otherwise remove it. */
    public static void toggle(String bungalowUri, String pageTitle) {
        if (!SiteUri.isSiteUri(bungalowUri)) return;
        if (isBookmarked(bungalowUri)) remove(bungalowUri);
        else                          add(bungalowUri, pageTitle);
    }

    // ── add ─────────────────────────────────────────────────────────────────────

    private static void add(String bungalowUri, String pageTitle) {
        String host    = SiteUri.host(bungalowUri);
        String rootUri = SiteUri.rootUri(bungalowUri);
        try {
            if (SiteUri.isRoot(bungalowUri)) {
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
                if (find(bungalowUri) == null) {
                    Bookmark child = new Bookmark(host, bungalowUri, label(pageTitle, pathOf(bungalowUri, host)));
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

    private static void remove(String bungalowUri) {
        try {
            if (SiteUri.isRoot(bungalowUri)) {
                Bookmark root = find(SiteUri.rootUri(bungalowUri));
                if (root == null) return;
                if (childCount(root) > 0) {     // keep as a container for its children
                    root.setAutoCreated(true);
                    dao().update(root);
                } else {
                    dao().delete(root);
                }
            } else {
                Bookmark child = find(bungalowUri);
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

    private static String pathOf(String bungalowUri, String host) {
        String prefix = SiteUri.PREFIX + host;
        String rest = bungalowUri.startsWith(prefix) ? bungalowUri.substring(prefix.length()) : bungalowUri;
        return rest.isBlank() ? host : rest.replaceFirst("^:", "");
    }
}
