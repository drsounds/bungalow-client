#include "web/BookmarkManager.h"

#include "web/BookmarkEvents.h"

#include "SPAppManager.h"

namespace space {
namespace web {
namespace {

const std::string kAppPrefix = "bungalow:app:";
const std::string kSitePrefix = "bungalow:site:";

bool startsWith(const std::string& s, const std::string& prefix) {
    return s.rfind(prefix, 0) == 0;
}

} // namespace

bool BookmarkManager::isBookmarked(Database& db, const std::string& uri) {
    return db.isBookmarked(uri);
}

bool BookmarkManager::toggle(Database& db, const std::string& uri, const std::string& title) {
    if (db.isBookmarked(uri)) {
        remove(db, uri);
        return false;
    }

    const std::string parent = domainUri(uri);
    if (parent.empty()) {
        db.addBookmark(uri, title);
    } else {
        if (!db.isBookmarked(parent)) db.addBookmark(parent, parent);
        db.addBookmark(uri, title, db.getBookmarkId(parent));
    }
    BookmarkEvents::fireChanged({uri, true});
    return true;
}

void BookmarkManager::remove(Database& db, const std::string& uri) {
    db.removeBookmark(uri);
    BookmarkEvents::fireChanged({uri, false});
}

std::vector<Database::Bookmark> BookmarkManager::roots(Database& db, const std::string& uriPrefix) {
    return db.getTopLevelBookmarks(uriPrefix);
}

std::vector<Database::Bookmark> BookmarkManager::childrenOf(Database& db, const Database::Bookmark& root) {
    return db.getChildBookmarks(root.id);
}

std::string BookmarkManager::displayTitle(const Database::Bookmark& bookmark) {
    const std::string& uri = bookmark.uri;
    if (!bookmark.title.empty() && bookmark.title != uri) return bookmark.title;

    std::string prefix;
    if (startsWith(uri, kAppPrefix)) prefix = kAppPrefix;
    else if (startsWith(uri, kSitePrefix)) prefix = kSitePrefix;
    else return uri;

    const std::string remainder = uri.substr(prefix.size());
    if (remainder.find(':') != std::string::npos) return uri; // sub-page: keep full URI
    if (prefix == kAppPrefix) {
        if (const SPAppBundle* bundle = SPAppManager::instance().findBundle(remainder)) {
            std::string name = bundle->getName();
            if (!name.empty()) return name;
        }
    }
    return remainder.empty() ? uri : remainder;
}

std::string BookmarkManager::domainUri(const std::string& uri) {
    // The root ends at the 3rd colon (after "bungalow:scheme:domain").
    const size_t first = uri.find(':');
    if (first == std::string::npos) return {};
    const size_t second = uri.find(':', first + 1);
    if (second == std::string::npos) return {};
    const size_t third = uri.find(':', second + 1);
    if (third == std::string::npos) return {};
    return uri.substr(0, third);
}

std::string BookmarkManager::faviconDomain(const std::string& uri) {
    struct SchemeEntry {
        const char* prefix;
        const char* hosted;
    };
    static const SchemeEntry kSchemes[] = {
        {"bungalow:site:", nullptr},
        {"bungalow:app:", ".apps.bungalow.se"},
        {"bungalow:curator:", ".curator.bungalow.se"},
        {"bungalow:store:", ".store.bungalow.se"},
    };

    for (const auto& scheme : kSchemes) {
        if (!startsWith(uri, scheme.prefix)) continue;
        const std::string remainder = uri.substr(std::char_traits<char>::length(scheme.prefix));
        std::string domain = remainder.substr(0, remainder.find(':'));
        // Installed app bundles are served locally and have no web favicon.
        if (startsWith(uri, kAppPrefix) && SPAppManager::instance().findBundle(domain)) return {};
        if (scheme.hosted && !domain.empty() && domain.find('.') == std::string::npos)
            domain += scheme.hosted;
        return domain;
    }
    return {};
}

} // namespace web
} // namespace space
