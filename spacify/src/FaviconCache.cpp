#include "FaviconCache.h"

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/webrequest.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace space {

// ── Cache directory ───────────────────────────────────────────────────────────

std::string FaviconCache::cacheDir() {
    static std::string dir;
    if (dir.empty()) {
        wxString dataDir = wxStandardPaths::Get().GetUserDataDir();
        dir = dataDir.ToStdString() + "/favicons";
        fs::create_directories(dir);
    }
    return dir;
}

// ── Construction / Destruction ────────────────────────────────────────────────

FaviconCache::FaviconCache() {
    Bind(wxEVT_WEBREQUEST_STATE, &FaviconCache::onRequestState, this);
    Bind(wxEVT_WEBREQUEST_DATA,  &FaviconCache::onRequestData,  this);
}

FaviconCache::~FaviconCache() {
    // Cancel all in-flight requests and discard callbacks so no stale
    // references fire after the owning MainElement is destroyed.
    for (auto& [domain, req] : m_inFlight)
        req.Cancel();
    m_requests.clear();
    m_inFlight.clear();
}

// ── Query ─────────────────────────────────────────────────────────────────────

std::string FaviconCache::getCachedPath(const std::string& domain) const {
    std::string path = cacheDir() + "/" + domain + ".ico";
    return fs::exists(path) ? path : "";
}

// ── Fetch ─────────────────────────────────────────────────────────────────────

void FaviconCache::fetchIfNeeded(const std::string& domain,
                                 const std::string& faviconUrl,
                                 std::function<void()> onReady)
{
    // Already on disk
    if (!getCachedPath(domain).empty()) return;

    // Already in flight
    if (m_inFlight.count(domain)) return;

    std::string destPath = cacheDir() + "/" + domain + ".ico";
    std::string tmpPath  = destPath + ".part";

    wxWebRequest req = wxWebSession::GetDefault().CreateRequest(
        this, wxString::FromUTF8(faviconUrl));
    if (!req.IsOk()) return;

    int evtId = req.GetId();

    FetchContext ctx;
    ctx.domain   = domain;
    ctx.destPath = destPath;
    ctx.tmpPath  = tmpPath;
    ctx.onReady  = std::move(onReady);
    ctx.file.open(tmpPath, std::ios::binary | std::ios::trunc);
    if (!ctx.file.is_open()) return;

    m_requests[evtId]    = std::move(ctx);
    m_inFlight[domain]   = req;
    req.Start();
}

// ── wxWebRequest event handlers ───────────────────────────────────────────────

void FaviconCache::onRequestData(wxWebRequestEvent& evt) {
    auto it = m_requests.find(evt.GetId());
    if (it == m_requests.end()) return;

    it->second.file.write(
        reinterpret_cast<const char*>(evt.GetDataBuffer()),
        static_cast<std::streamsize>(evt.GetDataSize()));
}

void FaviconCache::onRequestState(wxWebRequestEvent& evt) {
    auto it = m_requests.find(evt.GetId());
    if (it == m_requests.end()) return;

    FetchContext& ctx = it->second;

    if (evt.GetState() == wxWebRequest::State_Completed) {
        ctx.file.close();
        // Rename tmp → dest
        std::error_code ec;
        fs::rename(ctx.tmpPath, ctx.destPath, ec);

        if (!ec) {
            // Capture callback and clean up before invoking, so any rebuild
            // triggered by cb() sees a clean state (not in-flight).
            auto cb = ctx.onReady;
            m_inFlight.erase(ctx.domain);
            m_requests.erase(it);
            cb();
        } else {
            fs::remove(ctx.tmpPath, ec);
            m_inFlight.erase(ctx.domain);
            m_requests.erase(it);
        }
    } else if (evt.GetState() == wxWebRequest::State_Failed  ||
               evt.GetState() == wxWebRequest::State_Cancelled)
    {
        ctx.file.close();
        std::error_code ec;
        fs::remove(ctx.tmpPath, ec);
        m_inFlight.erase(ctx.domain);
        m_requests.erase(it);
    }
}

} // namespace space
