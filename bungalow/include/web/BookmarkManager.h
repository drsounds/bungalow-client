#pragma once

#include "Database.h"

#include <string>
#include <vector>

namespace space {
namespace web {

/// Toggles and queries hierarchical bookmarks of sites and apps.
///
/// A root bookmark represents a host (bungalow:site:example.com) or an app
/// (bungalow:app:finder); bookmarked sub-pages are its children. Bookmarking
/// a sub-page creates its root as a container if needed. Every change fires
/// BookmarkEvents.
class BookmarkManager {
public:
    BookmarkManager() = delete;

    static bool isBookmarked(Database& db, const std::string& uri);
    /// Adds the bookmark if absent, otherwise removes it. Returns true if added.
    static bool toggle(Database& db, const std::string& uri, const std::string& title);
    static void remove(Database& db, const std::string& uri);

    /// Top-level bookmarks whose URI starts with uriPrefix, for the sidebar.
    static std::vector<Database::Bookmark> roots(Database& db, const std::string& uriPrefix);
    static std::vector<Database::Bookmark> childrenOf(Database& db, const Database::Bookmark& root);

    /// Sidebar label for a bookmark. Container roots store their URI as the
    /// title; show the installed app's name or the bare host instead.
    static std::string displayTitle(const Database::Bookmark& bookmark);

    /// The root URI of a sub-page (bungalow:site:example.com:page ->
    /// bungalow:site:example.com), or "" if uri already is a root.
    static std::string domainUri(const std::string& uri);

    /// Host to fetch the favicon from for a site, app, curator or store URI;
    /// bare app/curator/store names get their hosted bungalow.se domain.
    /// "" for URIs without a web host, including installed app bundles.
    static std::string faviconDomain(const std::string& uri);
};

} // namespace web
} // namespace space
