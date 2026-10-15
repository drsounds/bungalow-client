#pragma once

#include <sqlite3.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

enum class DLStatus { Ongoing, Completed, Failed, Cancelled };

struct DownloadItem {
    int64_t     id           = 0;
    std::string url;
    std::string filename;
    std::string filePath;
    std::string contentType;
    DLStatus    status       = DLStatus::Ongoing;
    int64_t     totalBytes   = 0;
    int64_t     doneBytes    = 0;
    std::string errorMsg;

    float progress() const {
        return (totalBytes > 0) ? float(doneBytes) / float(totalBytes) : 0.f;
    }
};

// WinHTTP-backed download manager. Network I/O happens on worker threads;
// every state change and callback is delivered on the UI thread.
class DownloadManager {
public:
    using CheckCallback = std::function<void(const std::string& url,
                                             bool               isDownload,
                                             const std::string& filename,
                                             const std::string& contentType)>;

    explicit DownloadManager(sqlite3* db);
    ~DownloadManager();

    void checkUrl(const std::string& url, CheckCallback cb);

    int64_t startDownload(const std::string& url,
                          const std::string& filename,
                          const std::string& contentType);

    void cancelDownload(int64_t id);

    const std::vector<DownloadItem>& items() const { return m_items; }

    void setUpdateCallback(std::function<void()> cb) { m_onUpdate = cb; }

    static std::string getDownloadsFolder();
    static std::string getMusicFolder();

private:
    sqlite3* m_db = nullptr;

    std::vector<DownloadItem> m_items;
    int64_t m_nextItemId = 1;

    std::shared_ptr<bool> m_alive = std::make_shared<bool>(true);
    std::map<int64_t, std::shared_ptr<std::atomic<bool>>> m_cancelFlags;

    std::function<void()> m_onUpdate;

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
