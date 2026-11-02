#pragma once

#include <functional>
#include <string>

namespace space {
namespace web {

struct BookmarkChange {
    std::string uri;
    bool added = false;
};

/// Change notifications for bookmarks. BookmarkManager fires them after every
/// change; the web concept listens to keep the sidebar and favicons current.
/// UI thread only.
class BookmarkEvents {
public:
    using Listener = std::function<void(const BookmarkChange&)>;

    BookmarkEvents() = delete;

    /// Returns an id for removeListener().
    static int addListener(Listener listener);
    static void removeListener(int id);
    static void fireChanged(const BookmarkChange& change);
};

} // namespace web
} // namespace space
