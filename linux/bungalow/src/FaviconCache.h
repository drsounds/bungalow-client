#pragma once
#include <string>
#include <functional>
#include <map>
#include <fstream>

#include <wx/event.h>
#include <wx/webrequest.h>

namespace space {

// Fetches and caches favicons from the network.
//
// getCachedPath(domain)  — returns the on-disk path if already downloaded,
//                          or an empty string if not yet available.
// fetchIfNeeded(domain, faviconUrl, onReady)
//                        — no-op if already cached; otherwise issues an async
//                          GET for faviconUrl and calls onReady() on the main
//                          thread when the file has been saved to disk.
class FaviconCache : public wxEvtHandler {
public:
    FaviconCache();
    ~FaviconCache();

    // Returns the cached icon path for `domain`, or "" if not yet downloaded.
    std::string getCachedPath(const std::string& domain) const;

    // Kicks off a background fetch unless the favicon is already on disk or a
    // fetch is already in flight for this domain.
    void fetchIfNeeded(const std::string& domain,
                       const std::string& faviconUrl,
                       std::function<void()> onReady);

    // Returns the directory where favicons are stored, creating it if needed.
    static std::string cacheDir();

private:
    struct FetchContext {
        std::string domain;
        std::string destPath;
        std::string tmpPath;
        std::function<void()> onReady;
        std::ofstream file;
    };

    void onRequestState(wxWebRequestEvent& evt);
    void onRequestData(wxWebRequestEvent& evt);

    std::map<int, FetchContext> m_requests;   // eventId → context
    std::map<std::string, wxWebRequest> m_inFlight; // domain → live request
};

} // namespace space
