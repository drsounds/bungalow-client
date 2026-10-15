#include "DownloadManager.h"
#include "ConfigView.h"

#include <nlohmann/json.hpp>

#include <wx/filename.h>
#include <wx/stdpaths.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

// ── Audio extension list (mirrors MusicScanner) ───────────────────────────────

static const char* kAudioExts[] = {
    ".mp3", ".mp4", ".m4a", ".aac", ".wav", ".flac", ".ogg", ".opus", ".wma", nullptr
};

// ── Extensions that suggest a downloadable resource ──────────────────────────

static const char* kDownloadExts[] = {
    ".mp3", ".mp4", ".m4a", ".aac", ".wav", ".flac", ".ogg", ".opus", ".wma",
    ".zip", ".tar", ".gz", ".bz2", ".xz", ".rar", ".7z",
    ".pdf", ".dmg", ".pkg", ".exe", ".deb", ".rpm",
    ".avi", ".mkv", ".mov", ".wmv", ".webm",
    nullptr
};

// ── Audio content-type prefixes ───────────────────────────────────────────────

static const char* kAudioMimeTypes[] = {
    "audio/", "video/", nullptr  // video files may also be audio-only (mp4, mkv)
};

// ── Construction ──────────────────────────────────────────────────────────────

DownloadManager::DownloadManager(sqlite3* db) : m_db(db) {
    Bind(wxEVT_WEBREQUEST_STATE, &DownloadManager::onRequestState, this);
    Bind(wxEVT_WEBREQUEST_DATA,  &DownloadManager::onRequestData,  this);
    // Load persisted downloads from DB
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
            // Was in-progress when app quit — mark failed
            item.status = DLStatus::Failed;
            item.errorMsg = "Interrupted";
        }

        if (item.id >= m_nextItemId) m_nextItemId = item.id + 1;
        m_items.push_back(std::move(item));
    }
    sqlite3_finalize(stmt);
}

DownloadManager::~DownloadManager() = default;

// ── URL check ─────────────────────────────────────────────────────────────────

void DownloadManager::checkUrl(const std::string& url, CheckCallback cb) {
    // Fast path: URLs with no download-like extension go straight to the webview
    if (!mightBeDownload(url)) {
        cb(url, false, "", "");
        return;
    }

    int evtId = wxNewId();
    m_headChecks[evtId] = { url, cb };

    wxWebRequest req = wxWebSession::GetDefault().CreateRequest(this, url, evtId);
    req.SetMethod("HEAD");
    m_requests[evtId] = req;
    req.Start();
}

// ── Start download ────────────────────────────────────────────────────────────

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

    int evtId = wxNewId();

    // Open a temp file for writing
    std::string tmpPath = uniqueFilePath(getDownloadsFolder(), "." + item.filename + ".part");

    ActiveDL ctx;
    ctx.itemId  = item.id;
    ctx.tmpPath = tmpPath;
    ctx.file.open(tmpPath, std::ios::binary | std::ios::trunc);

    if (!ctx.file.is_open()) {
        // Can't open file — mark failed immediately
        m_items.back().status   = DLStatus::Failed;
        m_items.back().errorMsg = "Cannot create file";
        persistUpdate(m_items.back());
        notifyUpdate();
        return item.id;
    }

    m_activeReqs[evtId] = std::move(ctx);

    wxWebRequest req = wxWebSession::GetDefault().CreateRequest(this, url, evtId);
    m_requests[evtId] = req;
    req.Start();

    return item.id;
}

// ── Cancel ────────────────────────────────────────────────────────────────────

void DownloadManager::cancelDownload(int64_t id) {
    for (auto& [evtId, ctx] : m_activeReqs) {
        if (ctx.itemId == id) {
            auto it = m_requests.find(evtId);
            if (it != m_requests.end()) it->second.Cancel();
            break;
        }
    }
}

// ── wxWebRequest event handlers ───────────────────────────────────────────────

void DownloadManager::onRequestState(wxWebRequestEvent& evt) {
    int evtId = evt.GetId();

    // ── HEAD check result ─────────────────────────────────────────────────
    {
        auto it = m_headChecks.find(evtId);
        if (it != m_headChecks.end()) {
            HeadCheck check = std::move(it->second);
            m_headChecks.erase(it);
            m_requests.erase(evtId);

            if (evt.GetState() == wxWebRequest::State_Completed) {
                auto resp = evt.GetRequest().GetResponse();
                std::string ct   = resp.GetHeader("Content-Type").ToStdString();
                std::string disp = resp.GetHeader("Content-Disposition").ToStdString();
                int         code = resp.GetStatus();

                bool isDownload = (code >= 200 && code < 300) &&
                    (disp.find("attachment") != std::string::npos ||
                     isAudioContentType(ct) ||
                     ct.find("application/octet-stream") != std::string::npos);

                std::string filename = guessFilename(check.url, disp);
                check.cb(check.url, isDownload, filename, ct);
            } else {
                // HEAD failed or was cancelled — let webview load the URL normally
                check.cb(check.url, false, "", "");
            }
            return;
        }
    }

    // ── Active download state ─────────────────────────────────────────────
    {
        auto it = m_activeReqs.find(evtId);
        if (it == m_activeReqs.end()) return;

        ActiveDL& ctx = it->second;
        DownloadItem* item = findItem(ctx.itemId);
        if (!item) return;

        auto state = evt.GetState();

        if (state == wxWebRequest::State_Active) {
            // Update expected total from Content-Length if available
            auto resp = evt.GetRequest().GetResponse();
            if (item->totalBytes == 0) {
                auto expected = evt.GetRequest().GetBytesExpectedToReceive();
                if (expected > 0) item->totalBytes = expected;
            }

        } else if (state == wxWebRequest::State_Completed) {
            ctx.file.close();
            onDownloadComplete(*item, ctx.tmpPath);
            m_activeReqs.erase(it);
            m_requests.erase(evtId);

        } else if (state == wxWebRequest::State_Failed) {
            ctx.file.close();
            fs::remove(ctx.tmpPath);
            item->status   = DLStatus::Failed;
            item->errorMsg = evt.GetErrorDescription().ToStdString();
            persistUpdate(*item);
            m_activeReqs.erase(it);
            m_requests.erase(evtId);
            notifyUpdate();

        } else if (state == wxWebRequest::State_Cancelled) {
            ctx.file.close();
            fs::remove(ctx.tmpPath);
            item->status = DLStatus::Cancelled;
            persistUpdate(*item);
            m_activeReqs.erase(it);
            m_requests.erase(evtId);
            notifyUpdate();
        }
    }
}

void DownloadManager::onRequestData(wxWebRequestEvent& evt) {
    int evtId = evt.GetId();

    auto it = m_activeReqs.find(evtId);
    if (it == m_activeReqs.end()) return;

    ActiveDL& ctx = it->second;
    DownloadItem* item = findItem(ctx.itemId);
    if (!item) return;

    // Write chunk to file
    const void* buf  = evt.GetDataBuffer();
    std::size_t size = evt.GetDataSize();
    if (buf && size > 0) {
        ctx.file.write(static_cast<const char*>(buf), static_cast<std::streamsize>(size));
        item->doneBytes += static_cast<int64_t>(size);
    }

    // Update total if now known
    if (item->totalBytes == 0) {
        auto expected = evt.GetRequest().GetBytesExpectedToReceive();
        if (expected > 0) item->totalBytes = expected;
    }

    notifyUpdate();
}

// ── Completion handling ───────────────────────────────────────────────────────

void DownloadManager::onDownloadComplete(DownloadItem& item, const std::string& tmpPath) {
    std::string destDir  = getDownloadsFolder();
    std::string destPath = uniqueFilePath(destDir, item.filename);

    // Ensure destination directory exists
    fs::create_directories(destDir);

    std::error_code ec;
    fs::rename(tmpPath, destPath, ec);
    if (ec) {
        // Rename across volumes fails; fall back to copy + delete
        ec.clear();
        fs::copy_file(tmpPath, destPath, fs::copy_options::overwrite_existing, ec);
        if (ec) {
            // Copy failed — mark download as failed and clean up
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

    // Update DB with completed_at
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

    // Auto-import if audio
    if (isAudioFilename(item.filename) || isAudioContentType(item.contentType)) {
        std::string musicDir = getMusicFolder();
        fs::create_directories(musicDir);

        std::string musicPath = uniqueFilePath(musicDir, item.filename);
        fs::copy_file(destPath, musicPath,
                      fs::copy_options::overwrite_existing, ec);
        if (!ec) importAudioFile(musicPath);
    }

    notifyUpdate();
}

// ── Audio import ──────────────────────────────────────────────────────────────

void DownloadManager::importAudioFile(const std::string& filePath) {
    if (!m_db) return;

    sqlite3_stmt* stmt = nullptr;

    // Insert into files (ignore if already present)
    if (sqlite3_prepare_v2(m_db,
            "INSERT OR IGNORE INTO files(path) VALUES(?)", -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, filePath.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    // Retrieve file id
    int64_t fileId = 0;
    if (sqlite3_prepare_v2(m_db,
            "SELECT id FROM files WHERE path=?", -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, filePath.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) fileId = sqlite3_column_int64(stmt, 0);
        sqlite3_finalize(stmt);
    }

    if (fileId == 0) return;

    // Insert into audio_files with stem as name
    std::string name = fs::path(filePath).stem().string();
    if (sqlite3_prepare_v2(m_db,
            "INSERT OR IGNORE INTO audio_files(file_id, name) VALUES(?,?)",
            -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, fileId);
        sqlite3_bind_text (stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    // Insert into playables
    std::string uri = "file://" + filePath;
    if (sqlite3_prepare_v2(m_db,
            "INSERT OR IGNORE INTO playables(uri) VALUES(?)", -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, uri.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

// ── Database helpers ──────────────────────────────────────────────────────────

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

// ── Lookup ────────────────────────────────────────────────────────────────────

DownloadItem* DownloadManager::findItem(int64_t id) {
    for (auto& item : m_items)
        if (item.id == id) return &item;
    return nullptr;
}

void DownloadManager::notifyUpdate() {
    if (m_onUpdate) m_onUpdate();
}

// ── Static helpers ────────────────────────────────────────────────────────────

bool DownloadManager::mightBeDownload(const std::string& url) {
    // Strip query string
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
    // Try Content-Disposition: attachment; filename="foo.mp3"
    if (!contentDisposition.empty()) {
        auto pos = contentDisposition.find("filename=");
        if (pos != std::string::npos) {
            std::string fn = contentDisposition.substr(pos + 9);
            // Strip surrounding quotes
            if (!fn.empty() && fn.front() == '"') fn = fn.substr(1);
            auto endQ = fn.find('"');
            if (endQ != std::string::npos) fn = fn.substr(0, endQ);
            if (!fn.empty()) return fn;
        }
    }

    // Fall back to URL basename
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

    // Split stem and extension
    auto dot = filename.rfind('.');
    std::string stem = (dot != std::string::npos) ? filename.substr(0, dot) : filename;
    std::string ext  = (dot != std::string::npos) ? filename.substr(dot) : "";

    for (int i = 2; i < 1000; ++i) {
        std::string candidate = dir + "/" + stem + " (" + std::to_string(i) + ")" + ext;
        if (!fs::exists(candidate)) return candidate;
    }
    return path;  // fallback (will overwrite)
}

// ── OS folder paths ───────────────────────────────────────────────────────────

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
    return wxGetHomeDir().ToStdString() + "/Downloads";
}

std::string DownloadManager::getMusicFolder() {
    std::string v = readStringSetting("musicFolder");
    if (!v.empty()) return v;
    return wxGetHomeDir().ToStdString() + "/Music";
}
