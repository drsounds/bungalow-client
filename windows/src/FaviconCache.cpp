#include "FaviconCache.h"

#include "PlatformUi.h"
#include "UiDispatcher.h"
#include "WinHttpClient.h"

#include <filesystem>
#include <fstream>
#include <thread>

namespace fs = std::filesystem;

namespace space {

std::string FaviconCache::cacheDir() {
    static std::string dir;
    if (dir.empty()) {
        dir = platform::userDataDir() + "/favicons";
        std::error_code ec;
        fs::create_directories(dir, ec);
    }
    return dir;
}

FaviconCache::FaviconCache() = default;

FaviconCache::~FaviconCache() {
    *m_alive = false;
}

std::string FaviconCache::getCachedPath(const std::string& domain) const {
    std::string path = cacheDir() + "/" + domain + ".ico";
    return fs::exists(path) ? path : "";
}

void FaviconCache::fetchIfNeeded(const std::string& domain,
                                 const std::string& faviconUrl,
                                 std::function<void()> onReady)
{
    if (!getCachedPath(domain).empty()) return;
    if (m_inFlight.count(domain)) return;
    m_inFlight.insert(domain);

    const std::string destPath = cacheDir() + "/" + domain + ".ico";
    std::weak_ptr<bool> alive = m_alive;

    std::thread([this, alive, domain, faviconUrl, destPath, onReady = std::move(onReady)]() {
        const std::string tmpPath = destPath + ".part";
        bool saved = false;
        {
            std::ofstream file(tmpPath, std::ios::binary | std::ios::trunc);
            if (file.is_open()) {
                auto res = win::httpRequest(faviconUrl, "GET", [&](const char* d, size_t n) {
                    file.write(d, static_cast<std::streamsize>(n));
                    return true;
                });
                file.close();
                saved = res.ok && res.status >= 200 && res.status < 300;
            }
        }
        std::error_code ec;
        if (saved) fs::rename(tmpPath, destPath, ec);
        else fs::remove(tmpPath, ec);
        const bool success = saved && !ec;

        win::postToUi([this, alive, domain, success, onReady]() {
            auto token = alive.lock();
            if (!token || !*token) return;
            m_inFlight.erase(domain);
            if (success && onReady) onReady();
        });
    }).detach();
}

} // namespace space
