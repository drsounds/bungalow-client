#include "DownloadManager.h"
#include "ConfigView.h"
#include "PlatformUi.h"
#include "UiDispatcher.h"
#include "WinHttpClient.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>

namespace fs = std::filesystem;

static const char* kAudioExts[] = {
    ".mp3", ".mp4", ".m4a", ".aac", ".wav", ".flac", ".ogg", ".opus", ".wma", nullptr
};

static const char* kDownloadExts[] = {
    ".mp3", ".mp4", ".m4a", ".aac", ".wav", ".flac", ".ogg", ".opus", ".wma",
    ".zip", ".tar", ".gz", ".bz2", ".xz", ".rar", ".7z",
    ".pdf", ".dmg", ".pkg", ".exe", ".deb", ".rpm",
    ".avi", ".mkv", ".mov", ".wmv", ".webm",
    nullptr
};

static const char* kAudioMimeTypes[] = { "audio/", "video/", nullptr };

DownloadManager::DownloadManager(sqlite3* db) : m_db(db) {
    if (!m_db) return;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, url, filename, file_path, content_type, "
                      "status, total_bytes, done_bytes FROM downloads ORDER BY id";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return;

    auto col = [&](int i) -> std::string {
        auto* t = sqlite3_column_text(stmt, i);
        return t ? reinterpret_cast<const char*>(t) : "";
    };

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        DownloadItem item;
        item.id          = sqlite3_column_int64(stmt, 0);
        item.url         = col(1);
        item.filename    = col(2);
        item.filePath    = col(3);
        item.contentType = col(4);
        std::string st   = col(5);
        item.totalBytes  = sqlite3_column_int64(stmt, 6);
        item.doneBytes   = sqlite3_column_int64(stmt, 7);

        if      (st == "completed")  item.status = DLStatus::Completed;
        else if (st == "failed")     item.status = DLStatus::Failed;
        else if (st == "cancelled")  item.status = DLStatus::Cancelled;
        else {
            item.status = DLStatus::Failed;
            item.errorMsg = "Interrupted";
        }

        if (item.id >= m_nextItemId) m_nextItemId = item.id + 1;
        m_items.push_back(std::move(item));
    }
    sqlite3_finalize(stmt);
}

DownloadManager::~DownloadManager() {
    *m_alive = false;
    for (auto& [id, flag] : m_cancelFlags) flag->store(true);
}

void DownloadManager::checkUrl(const std::string& url, CheckCallback cb) {
    if (!mightBeDownload(url)) {
        cb(url, false, "", "");
        return;
    }

    std::weak_ptr<bool> alive = m_alive;
    std::thread([alive, url, cb = std::move(cb)]() {
        auto res = space::win::httpRequest(url, "HEAD", nullptr);
        bool isDownload = false;
        std::string filename;
        if (res.ok) {
            isDownload = (res.status >= 200 && res.status < 300) &&
                (res.contentDisposition.find("attachment") != std::string::npos ||
                 isAudioContentType(res.contentType) ||
                 res.contentType.find("application/octet-stream") != std::string::npos);
            filename = guessFilename(url, res.contentDisposition);
        }
        std::string ct = res.contentType;
        space::win::postToUi([alive, url, cb, isDownload, filename, ct]() {
            auto token = alive.lock();
            if (!token || !*token) return;
            cb(url, isDownload, isDownload ? filename : "", isDownload ? ct : "");
        });
    }).detach();
}

int64_t DownloadManager::startDownload(const std::string& url,
                                        const std::string& filename,
                                        const std::string& contentType) {
    DownloadItem item;
    item.id          = m_nextItemId++;
    item.url         = url;
    item.filename    = filename.empty() ? guessFilename(url, "") : filename;
    item.contentType = contentType;
    item.status      = DLStatus::Ongoing;

    persistNew(item);
    m_items.push_back(item);
    notifyUpdate();

    const int64_t id = item.id;
    std::error_code ec;
    fs::create_directories(getDownloadsFolder(), ec);
    std::string tmpPath = uniqueFilePath(getDownloadsFolder(), "." + item.filename + ".part");

    auto cancel = std::make_shared<std::atomic<bool>>(false);
    m_cancelFlags[id] = cancel;

    std::weak_ptr<bool> alive = m_alive;
    std::thread([this, alive, id, url, tmpPath, cancel]() {
        std::ofstream file(tmpPath, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            space::win::postToUi([this, alive, id]() {
                auto token = alive.lock();
                if (!token || !*token) return;
                m_cancelFlags.erase(id);
                if (auto* it = findItem(id)) {
                    it->status = DLStatus::Failed;
                    it->errorMsg = "Cannot create file";
                    persistUpdate(*it);
                    notifyUpdate();
                }
            });
            return;
        }

        int64_t done = 0;
        int64_t total = 0;
        int chunks = 0;
        auto postProgress = [this, alive, id](int64_t doneNow, int64_t totalNow) {
            space::win::postToUi([this, alive, id, doneNow, totalNow]() {
                auto token = alive.lock();
                if (!token || !*token) return;
                if (auto* it = findItem(id)) {
                    if (it->status != DLStatus::Ongoing) return;
                    it->doneBytes = doneNow;
                    if (totalNow > 0) it->totalBytes = totalNow;
                    notifyUpdate();
                }
            });
        };

        auto res = space::win::httpRequest(url, "GET",
            [&](const char* d, size_t n) {
                file.write(d, static_cast<std::streamsize>(n));
                done += static_cast<int64_t>(n);
                if ((++chunks % 8) == 0) postProgress(done, total);
                return true;
            }, cancel.get(),
            [&](const space::win::HttpResult& h) { total = h.contentLength; });
        file.close();

        const bool ok = res.ok && !res.cancelled && res.status >= 200 && res.status < 300;
        const bool cancelled = res.cancelled;
        std::string err = res.ok ? ("HTTP " + std::to_string(res.status)) : res.error;
        if (!ok) {
            std::error_code rec;
            fs::remove(tmpPath, rec);
        }

        space::win::postToUi([this, alive, id, ok, cancelled, err, tmpPath, done, total]() {
            auto token = alive.lock();
            if (!token || !*token) return;
            m_cancelFlags.erase(id);
            DownloadItem* it = findItem(id);
            if (!it) return;
            it->doneBytes = done;
            if (total > 0) it->totalBytes = total;
            if (ok) {
                onDownloadComplete(*it, tmpPath);
            } else if (cancelled) {
                it->status = DLStatus::Cancelled;
                persistUpdate(*it);
                notifyUpdate();
            } else {
                it->status = DLStatus::Failed;
                it->errorMsg = err;
                persistUpdate(*it);
                notifyUpdate();
            }
        });
    }).detach();

    return id;
}

void DownloadManager::cancelDownload(int64_t id) {
    auto it = m_cancelFlags.find(id);
    if (it != m_cancelFlags.end()) it->second->store(true);
}

void DownloadManager::onDownloadComplete(DownloadItem& item, const std::string& tmpPath) {
    std::string destDir  = getDownloadsFolder();
    std::error_code ec;
    fs::create_directories(destDir, ec);
    std::string destPath = uniqueFilePath(destDir, item.filename);

    fs::rename(tmpPath, destPath, ec);
    if (ec) {
        ec.clear();
        fs::copy_file(tmpPath, destPath, fs::copy_options::overwrite_existing, ec);
        if (ec) {
            fs::remove(tmpPath);
            item.status   = DLStatus::Failed;
            item.errorMsg = ec.message();
            persistUpdate(item);
            notifyUpdate();
            return;
        }
        fs::remove(tmpPath);
    }

    item.filePath  = destPath;
    item.status    = DLStatus::Completed;
    item.doneBytes = item.totalBytes > 0 ? item.totalBytes : item.doneBytes;

    if (m_db) {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(m_db,
                "UPDATE downloads SET status='completed', file_path=?, done_bytes=?, "
                "total_bytes=?, completed_at=datetime('now') WHERE id=?",
                -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text (stmt, 1, item.filePath.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int64(stmt, 2, item.doneBytes);
            sqlite3_bind_int64(stmt, 3, item.totalBytes);
            sqlite3_bind_int64(stmt, 4, item.id);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    if (isAudioFilename(item.filename) || isAudioContentType(item.contentType)) {
        std::string musicDir = getMusicFolder();
        fs::create_directories(musicDir, ec);

        std::string musicPath = uniqueFilePath(musicDir, item.filename);
        fs::copy_file(destPath, musicPath, fs::copy_options::overwrite_existing, ec);
        if (!ec) importAudioFile(musicPath);
    }

    notifyUpdate();
}

void DownloadManager::importAudioFile(const std::string& filePath) {
    if (!m_db) return;

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_db,
            "INSERT OR IGNORE INTO files(path) VALUES(?)", -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, filePath.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    int64_t fileId = 0;
    if (sqlite3_prepare_v2(m_db,
            "SELECT id FROM files WHERE path=?", -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, filePath.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) fileId = sqlite3_column_int64(stmt, 0);
        sqlite3_finalize(stmt);
    }

    if (fileId == 0) return;

    std::string name = fs::path(filePath).stem().string();
    if (sqlite3_prepare_v2(m_db,
            "INSERT OR IGNORE INTO audio_files(file_id, name) VALUES(?,?)",
            -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, fileId);
        sqlite3_bind_text (stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    std::string uri = "file://" + filePath;
    if (sqlite3_prepare_v2(m_db,
            "INSERT OR IGNORE INTO playables(uri) VALUES(?)", -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, uri.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

void DownloadManager::persistNew(const DownloadItem& item) {
    if (!m_db) return;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db,
            "INSERT INTO downloads(id, url, filename, content_type, status, "
            "total_bytes, done_bytes) VALUES(?,?,?,?,'ongoing',0,0)",
            -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_int64(stmt, 1, item.id);
    sqlite3_bind_text (stmt, 2, item.url.c_str(),         -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, item.filename.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, item.contentType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void DownloadManager::persistUpdate(const DownloadItem& item) {
    if (!m_db) return;
    const char* statusStr = "ongoing";
    if      (item.status == DLStatus::Completed) statusStr = "completed";
    else if (item.status == DLStatus::Failed)    statusStr = "failed";
    else if (item.status == DLStatus::Cancelled) statusStr = "cancelled";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db,
            "UPDATE downloads SET status=?, total_bytes=?, done_bytes=?, file_path=? WHERE id=?",
            -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text (stmt, 1, statusStr,             -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, item.totalBytes);
    sqlite3_bind_int64(stmt, 3, item.doneBytes);
    sqlite3_bind_text (stmt, 4, item.filePath.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 5, item.id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

DownloadItem* DownloadManager::findItem(int64_t id) {
    for (auto& item : m_items)
        if (item.id == id) return &item;
    return nullptr;
}

void DownloadManager::notifyUpdate() {
    if (m_onUpdate) m_onUpdate();
}

bool DownloadManager::mightBeDownload(const std::string& url) {
    std::string path = url;
    auto q = path.find('?');
    if (q != std::string::npos) path = path.substr(0, q);

    auto dot = path.rfind('.');
    if (dot == std::string::npos) return false;

    std::string ext = path.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    for (int i = 0; kDownloadExts[i]; ++i)
        if (ext == kDownloadExts[i]) return true;
    return false;
}

std::string DownloadManager::guessFilename(const std::string& url,
                                            const std::string& contentDisposition) {
    if (!contentDisposition.empty()) {
        auto pos = contentDisposition.find("filename=");
        if (pos != std::string::npos) {
            std::string fn = contentDisposition.substr(pos + 9);
            if (!fn.empty() && fn.front() == '"') fn = fn.substr(1);
            auto endQ = fn.find('"');
            if (endQ != std::string::npos) fn = fn.substr(0, endQ);
            if (!fn.empty()) return fn;
        }
    }

    std::string path = url;
    auto q = path.find('?');
    if (q != std::string::npos) path = path.substr(0, q);
    auto slash = path.rfind('/');
    if (slash != std::string::npos) path = path.substr(slash + 1);
    return path.empty() ? "download" : path;
}

bool DownloadManager::isAudioContentType(const std::string& ct) {
    for (int i = 0; kAudioMimeTypes[i]; ++i)
        if (ct.find(kAudioMimeTypes[i]) == 0) return true;
    return false;
}

bool DownloadManager::isAudioFilename(const std::string& filename) {
    auto dot = filename.rfind('.');
    if (dot == std::string::npos) return false;
    std::string ext = filename.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    for (int i = 0; kAudioExts[i]; ++i)
        if (ext == kAudioExts[i]) return true;
    return false;
}

std::string DownloadManager::uniqueFilePath(const std::string& dir,
                                             const std::string& filename) {
    std::string path = dir + "/" + filename;
    if (!fs::exists(path)) return path;

    auto dot = filename.rfind('.');
    std::string stem = (dot != std::string::npos) ? filename.substr(0, dot) : filename;
    std::string ext  = (dot != std::string::npos) ? filename.substr(dot) : "";

    for (int i = 2; i < 1000; ++i) {
        std::string candidate = dir + "/" + stem + " (" + std::to_string(i) + ")" + ext;
        if (!fs::exists(candidate)) return candidate;
    }
    return path;
}

static std::string readStringSetting(const std::string& key) {
    std::ifstream f(space::views::ConfigView::getSettingsPath());
    if (!f.is_open()) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    try {
        auto obj = nlohmann::json::parse(ss.str());
        if (obj.contains(key))
            return obj.at(key).get<std::string>();
    } catch (...) {}
    return "";
}

std::string DownloadManager::getDownloadsFolder() {
    std::string v = readStringSetting("downloadsFolder");
    if (!v.empty()) return v;
    return space::platform::homeDir() + "/Downloads";
}

std::string DownloadManager::getMusicFolder() {
    std::string v = readStringSetting("musicFolder");
    if (!v.empty()) return v;
    return space::platform::homeDir() + "/Music";
}
