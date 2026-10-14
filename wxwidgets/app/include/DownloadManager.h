#pragma once

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/webrequest.h>

#include <sqlite3.h>

#include <functional>
#include <fstream>
#include <map>
#include <string>
#include <vector>

// ── Download status ───────────────────────────────────────────────────────────

enum class DLStatus { Ongoing, Completed, Failed, Cancelled };

// ── Download item ─────────────────────────────────────────────────────────────

struct DownloadItem {
    int64_t     id           = 0;
    std::string url;
    std::string filename;
    std::string filePath;      // final path after completion
    std::string contentType;
    DLStatus    status       = DLStatus::Ongoing;
    int64_t     totalBytes   = 0;
    int64_t     doneBytes    = 0;
    std::string errorMsg;

    float progress() const {
        return (totalBytes > 0) ? float(doneBytes) / float(totalBytes) : 0.f;
    }
};

// ── Download manager ──────────────────────────────────────────────────────────

class DownloadManager : public wxEvtHandler {
public:
    /// Callback fired on the main thread when a HEAD check completes.
    /// isDownload=true means the URL should be downloaded instead of navigated.
    using CheckCallback = std::function<void(const std::string& url,
                                             bool               isDownload,
                                             const std::string& filename,
                                             const std::string& contentType)>;

    explicit DownloadManager(sqlite3* db);
    ~DownloadManager() override;

    /// Check whether a URL points to a downloadable resource.
    /// Calls cb on the main thread with the result.
    /// If the URL has no download-like extension, cb is called immediately
    /// with isDownload=false to avoid the HEAD-request round-trip.
    void checkUrl(const std::string& url, CheckCallback cb);

    /// Start a background download. Returns the new item ID.
    int64_t startDownload(const std::string& url,
                          const std::string& filename,
                          const std::string& contentType);

    /// Cancel an ongoing download.
    void cancelDownload(int64_t id);

    /// All items (main-thread only).
    const std::vector<DownloadItem>& items() const { return m_items; }

    /// Register a callback invoked on every progress update or status change.
    void setUpdateCallback(std::function<void()> cb) { m_onUpdate = cb; }

    /// OS-default or user-configured download/music folders.
    static std::string getDownloadsFolder();
    static std::string getMusicFolder();

private:
    sqlite3* m_db = nullptr;

    std::vector<DownloadItem> m_items;
    int64_t m_nextItemId = 1;

    // ── HEAD checks ────────────────────────────────────────────────────────
    struct HeadCheck {
        std::string   url;
        CheckCallback cb;
    };
    std::map<int, HeadCheck>   m_headChecks;  // evtId -> check

    // ── Active downloads ───────────────────────────────────────────────────
    struct ActiveDL {
        int64_t      itemId;
        std::ofstream file;
        std::string  tmpPath;
    };
    std::map<int, ActiveDL>    m_activeReqs;  // evtId -> download context

    // wxWebRequest objects kept alive until completion
    std::map<int, wxWebRequest> m_requests;

    std::function<void()> m_onUpdate;

    // ── wxWebRequest event handlers ────────────────────────────────────────
    void onRequestState(wxWebRequestEvent& evt);
    void onRequestData(wxWebRequestEvent& evt);

    // ── Helpers ────────────────────────────────────────────────────────────
    DownloadItem* findItem(int64_t id);
    void notifyUpdate();
    void persistNew(const DownloadItem& item);
    void persistUpdate(const DownloadItem& item);
    void onDownloadComplete(DownloadItem& item, const std::string& tmpPath);
    void importAudioFile(const std::string& filePath);

    static bool        mightBeDownload(const std::string& url);
    static std::string guessFilename(const std::string& url,
                                     const std::string& contentDisposition);
    static bool        isAudioContentType(const std::string& ct);
    static bool        isAudioFilename(const std::string& filename);
    static std::string uniqueFilePath(const std::string& dir,
                                      const std::string& filename);
};
