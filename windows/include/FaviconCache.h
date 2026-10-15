#pragma once
#include <functional>
#include <memory>
#include <set>
#include <string>

namespace space {

// Fetches and caches favicons from the network (WinHTTP worker threads).
// Same public API as the wx implementation; onReady runs on the UI thread.
class FaviconCache {
public:
    FaviconCache();
    ~FaviconCache();

    std::string getCachedPath(const std::string& domain) const;

    void fetchIfNeeded(const std::string& domain,
                       const std::string& faviconUrl,
                       std::function<void()> onReady);

    static std::string cacheDir();

private:
    std::shared_ptr<bool> m_alive = std::make_shared<bool>(true);
    std::set<std::string> m_inFlight; // UI thread only
};

} // namespace space
