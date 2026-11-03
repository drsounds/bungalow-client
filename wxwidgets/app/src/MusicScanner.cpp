#include "MusicScanner.h"

#include "AudioImporter.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/msgdlg.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

#include <nlohmann/json.hpp>
#include <cstdint>

#include <filesystem>
#include <vector>
#include <algorithm>
#include <cstdio>

namespace fs = std::filesystem;

// ── Event definitions ─────────────────────────────────────────────────────────

wxDEFINE_EVENT(EVT_SCAN_PROGRESS, wxCommandEvent);
wxDEFINE_EVENT(EVT_SCAN_DONE,     wxCommandEvent);

// =============================================================================
// MusicScannerThread
// =============================================================================

// ── Audio extension sets ──────────────────────────────────────────────────────

static const char* kAudioExts[] = {
    ".mp3", ".mp4", ".m4a", ".aac", ".wav", ".flac", ".ogg", ".opus", ".wma", nullptr
};

bool MusicScannerThread::isAudioFile(const std::string& ext) {
    std::string lo = ext;
    std::transform(lo.begin(), lo.end(), lo.begin(), ::tolower);
    for (int i = 0; kAudioExts[i]; ++i)
        if (lo == kAudioExts[i]) return true;
    return false;
}

// ── URL encoding ──────────────────────────────────────────────────────────────

std::string MusicScannerThread::urlEncode(const std::string& s) {
    std::string out;
    out.reserve(s.size() * 3);
    for (unsigned char c : s) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out += char(c);
        } else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", c);
            out += buf;
        }
    }
    return out;
}

// ── HTTP GET via system curl ──────────────────────────────────────────────────

std::string MusicScannerThread::httpGet(const std::string& url) {
    // Escape single quotes in URL to be shell-safe
    std::string safe;
    safe.reserve(url.size());
    for (char c : url) {
        if (c == '\'') safe += "%27";
        else           safe += c;
    }
    std::string cmd =
        "curl -s --max-time 3 "
        "-A 'Bungalow/1.0 (https://bungalow.app)' "
        "'" + safe + "' 2>/dev/null";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return {};
    std::string result;
    char buf[4096];
    while (fgets(buf, sizeof(buf), pipe))
        result += buf;
    pclose(pipe);
    return result;
}

// ── SQLite helpers ────────────────────────────────────────────────────────────

sqlite3_int64 MusicScannerThread::upsertFile(sqlite3* db, const std::string& path) {
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(db,
        "INSERT OR IGNORE INTO files (path) VALUES (?)", -1, &st, nullptr);
    sqlite3_bind_text(st, 1, path.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(st);
    sqlite3_finalize(st);

    sqlite3_prepare_v2(db,
        "SELECT id FROM files WHERE path = ?", -1, &st, nullptr);
    sqlite3_bind_text(st, 1, path.c_str(), -1, SQLITE_STATIC);
    sqlite3_int64 id = -1;
    if (sqlite3_step(st) == SQLITE_ROW) id = sqlite3_column_int64(st, 0);
    sqlite3_finalize(st);
    return id;
}

sqlite3_int64 MusicScannerThread::upsertAudioFile(sqlite3* db, sqlite3_int64 fileId,
                                                    const AudioMetadata& meta) {
    // Return existing row if already imported
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(db,
        "SELECT id FROM audio_files WHERE file_id = ?", -1, &st, nullptr);
    sqlite3_bind_int64(st, 1, fileId);
    sqlite3_int64 existing = -1;
    if (sqlite3_step(st) == SQLITE_ROW) existing = sqlite3_column_int64(st, 0);
    sqlite3_finalize(st);
    if (existing >= 0) return existing;

    sqlite3_prepare_v2(db, R"(
        INSERT INTO audio_files (file_id, name, version, artists, album, duration)
        VALUES (?, ?, ?, ?, ?, ?)
    )", -1, &st, nullptr);
    sqlite3_bind_int64(st, 1, fileId);
    sqlite3_bind_text (st, 2, meta.title.c_str(),   -1, SQLITE_STATIC);
    if (meta.version.empty())
        sqlite3_bind_null(st, 3);
    else
        sqlite3_bind_text(st, 3, meta.version.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text (st, 4, meta.artists.c_str(),  -1, SQLITE_STATIC);
    sqlite3_bind_text (st, 5, meta.album.c_str(),    -1, SQLITE_STATIC);
    sqlite3_bind_int  (st, 6, meta.duration);
    int rc = sqlite3_step(st);
    sqlite3_int64 id = (rc == SQLITE_DONE) ? sqlite3_last_insert_rowid(db) : -1;
    sqlite3_finalize(st);
    return id;
}

sqlite3_int64 MusicScannerThread::upsertPlayable(sqlite3* db, const std::string& path) {
    std::string uri = "file://" + path;
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(db,
        "INSERT OR IGNORE INTO playables (uri) VALUES (?)", -1, &st, nullptr);
    sqlite3_bind_text(st, 1, uri.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(st);
    sqlite3_finalize(st);

    sqlite3_prepare_v2(db,
        "SELECT id FROM playables WHERE uri = ?", -1, &st, nullptr);
    sqlite3_bind_text(st, 1, uri.c_str(), -1, SQLITE_STATIC);
    sqlite3_int64 id = -1;
    if (sqlite3_step(st) == SQLITE_ROW) id = sqlite3_column_int64(st, 0);
    sqlite3_finalize(st);
    return id;
}

// Insert a save for trackId if no active save already exists.
static void addSave(sqlite3* db, sqlite3_int64 trackId) {
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(db,
        "INSERT INTO saves (track_id) "
        "SELECT ? WHERE NOT EXISTS ("
        "  SELECT 1 FROM saves WHERE track_id = ? AND deleted_at IS NULL"
        ")",
        -1, &st, nullptr);
    sqlite3_bind_int64(st, 1, trackId);
    sqlite3_bind_int64(st, 2, trackId);
    sqlite3_step(st);
    sqlite3_finalize(st);
}

// ── MusicBrainz lookup ────────────────────────────────────────────────────────

// True if the file at @p path is already linked to a track.
bool MusicScannerThread::isLinked(sqlite3* db, const std::string& path) {
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db, R"(
        SELECT 1 FROM files f
        JOIN audio_files af        ON af.file_id = f.id
        JOIN track_audio_files taf ON taf.audio_file_id = af.id
        WHERE f.path = ? LIMIT 1
    )", -1, &st, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(st, 1, path.c_str(), -1, SQLITE_TRANSIENT);
    const bool linked = sqlite3_step(st) == SQLITE_ROW;
    sqlite3_finalize(st);
    return linked;
}

// Store an untagged file and let MusicBrainz create its recording/release/track
// rows. Returns false (nothing linked) when the lookup finds nothing.
bool MusicScannerThread::identifyWithMusicBrainz(sqlite3* db, const std::string& path,
                                                  const space::AudioFileMetadata& tags) {
    AudioMetadata meta;
    meta.title    = tags.title;
    meta.version  = tags.version;
    meta.artists  = tags.artists;
    meta.album    = tags.album;
    meta.duration = tags.duration;

    sqlite3_int64 fileId = upsertFile(db, path);
    if (fileId < 0) return false;
    sqlite3_int64 afId = upsertAudioFile(db, fileId, meta);
    if (afId < 0) return false;
    sqlite3_int64 playableId = upsertPlayable(db, path);

    // Throttle to ≤ 1 MusicBrainz request per second (ToS requirement)
    for (int ms = 0; ms < 1100 && !TestDestroy(); ms += 100)
        wxThread::Sleep(100);
    return !TestDestroy() && musicBrainzLookup(db, meta, afId, playableId);
}

bool MusicScannerThread::musicBrainzLookup(sqlite3* db, const AudioMetadata& meta,
                                             sqlite3_int64 audioFileId,
                                             sqlite3_int64 playableId) {
    // Build query: recording:"Title" AND artist:"Artists"
    std::string query = "recording:\"" + meta.title + "\"";
    if (!meta.artists.empty())
        query += " AND artist:\"" + meta.artists + "\"";

    std::string url =
        "https://musicbrainz.org/ws/2/recording"
        "?query=" + urlEncode(query) +
        "&inc=releases+isrcs+artist-credits"
        "&fmt=json&limit=1";

    std::string json = httpGet(url);
    if (json.empty()) return false;

    return createFromMBJson(db, json, meta, audioFileId, playableId);
}

// Parse a MusicBrainz recording JSON response, create DB rows, and link.
bool MusicScannerThread::createFromMBJson(sqlite3* db, const std::string& jsonStr,
                                           const AudioMetadata& localMeta,
                                           sqlite3_int64 audioFileId,
                                           sqlite3_int64 playableId) {
    nlohmann::json jv;
    try { jv = nlohmann::json::parse(jsonStr); } catch (...) { return false; }

    auto* root = jv.is_object() ? &jv : nullptr;
    if (!root || !root->contains("recordings")) return false;
    auto* recs = root->at("recordings").is_array() ? &root->at("recordings") : nullptr;
    if (!recs || recs->empty()) return false;

    auto* rec = &(*recs)[0];
    if (!rec->is_object()) return false;

    // Require a minimum confidence score
    if (rec->contains("score")) {
        try {
            int score = rec->at("score").get<std::int64_t>();
            if (score < 85) return false;
        } catch (...) {}
    }

    // ── Recording identity ──────────────────────────────────────────────────
    std::string mbid, recTitle;
    try {
        mbid     = rec->at("id").get<std::string>();
        recTitle = rec->at("title").get<std::string>();
    } catch (...) { return false; }

    // Prefer real ISRC; fall back to "mb:<mbid>"
    std::string isrc = "mb:" + mbid;
    if (rec->contains("isrcs")) {
        auto* isrcs = rec->at("isrcs").is_array() ? &rec->at("isrcs") : nullptr;
        if (isrcs && !isrcs->empty()) {
            try { isrc = (*isrcs)[0].get<std::string>(); } catch (...) {}
        }
    }

    // ── Artists ─────────────────────────────────────────────────────────────
    std::string primaryArtistKey;
    auto upsertArtist = [&](const std::string& key, const std::string& name) {
        sqlite3_stmt* st = nullptr;
        sqlite3_prepare_v2(db,
            "INSERT OR IGNORE INTO artists (isni, name) VALUES (?, ?)",
            -1, &st, nullptr);
        sqlite3_bind_text(st, 1, key.c_str(),  -1, SQLITE_STATIC);
        sqlite3_bind_text(st, 2, name.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(st);
        sqlite3_finalize(st);
    };

    if (rec->contains("artist-credit")) {
        auto* credits = rec->at("artist-credit").is_array()
                            ? &rec->at("artist-credit") : nullptr;
        if (credits) {
            int num = 0;
            for (auto& c : *credits) {
                auto* co = c.is_object() ? &c : nullptr;
                if (!co || !co->contains("artist")) continue;
                auto* a = co->at("artist").is_object() ? &co->at("artist") : nullptr;
                if (!a) continue;

                std::string aId, aName;
                try {
                    aId   = "mb:" + a->at("id").get<std::string>();
                    aName = a->at("name").get<std::string>();
                } catch (...) { continue; }

                // Prefer ISNI if present
                if (a->contains("isnis")) {
                    auto* isnis = a->at("isnis").is_array() ? &a->at("isnis") : nullptr;
                    if (isnis && !isnis->empty()) {
                        try { aId = (*isnis)[0].get<std::string>(); }
                        catch (...) {}
                    }
                }
                if (num == 0) primaryArtistKey = aId;
                upsertArtist(aId, aName);

                // recording_artists junction
                sqlite3_stmt* st = nullptr;
                sqlite3_prepare_v2(db,
                    "INSERT OR IGNORE INTO recording_artists "
                    "(recording_id, artist_id, is_primary, number) VALUES (?, ?, ?, ?)",
                    -1, &st, nullptr);
                sqlite3_bind_text(st, 1, isrc.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(st, 2, aId.c_str(),  -1, SQLITE_STATIC);
                sqlite3_bind_int (st, 3, num == 0 ? 1 : 0);
                sqlite3_bind_int (st, 4, num);
                sqlite3_step(st);
                sqlite3_finalize(st);
                ++num;
            }
        }
    }

    // ── Recording row ────────────────────────────────────────────────────────
    {
        sqlite3_stmt* st = nullptr;
        sqlite3_prepare_v2(db,
            "INSERT OR IGNORE INTO recordings (isrc, name, version) VALUES (?, ?, ?)",
            -1, &st, nullptr);
        sqlite3_bind_text(st, 1, isrc.c_str(),    -1, SQLITE_STATIC);
        sqlite3_bind_text(st, 2, recTitle.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(st, 3,
            localMeta.version.empty() ? nullptr : localMeta.version.c_str(),
            -1, SQLITE_STATIC);
        sqlite3_step(st);
        sqlite3_finalize(st);
    }

    // ── Release ──────────────────────────────────────────────────────────────
    std::string releaseKey;
    if (rec->contains("releases")) {
        auto* rels = rec->at("releases").is_array() ? &rec->at("releases") : nullptr;
        if (rels && !rels->empty()) {
            auto* rel = &(*rels)[0];
            if (rel->is_object()) {
                std::string relMbid, relTitle, relDate;
                try {
                    relMbid  = rel->at("id").get<std::string>();
                    relTitle = rel->at("title").get<std::string>();
                } catch (...) {}

                if (rel->contains("date") && !(*rel)["date"].is_null()) {
                    try { relDate = rel->at("date").get<std::string>(); }
                    catch (...) {}
                }

                // Prefer UPC/barcode; fall back to "mb:<release-mbid>"
                releaseKey = "mb:" + relMbid;
                if (rel->contains("barcode") && !(*rel)["barcode"].is_null()) {
                    try {
                        std::string bc = rel->at("barcode").get<std::string>();
                        if (!bc.empty()) releaseKey = bc;
                    } catch (...) {}
                }

                sqlite3_stmt* st = nullptr;
                sqlite3_prepare_v2(db,
                    "INSERT OR IGNORE INTO releases (upc, name, release_date) "
                    "VALUES (?, ?, ?)",
                    -1, &st, nullptr);
                sqlite3_bind_text(st, 1, releaseKey.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(st, 2, relTitle.c_str(),   -1, SQLITE_STATIC);
                sqlite3_bind_text(st, 3,
                    relDate.empty() ? nullptr : relDate.c_str(), -1, SQLITE_STATIC);
                sqlite3_step(st);
                sqlite3_finalize(st);

                // release_artists: link primary artist to release
                if (!primaryArtistKey.empty()) {
                    sqlite3_prepare_v2(db,
                        "INSERT OR IGNORE INTO release_artists "
                        "(release_id, artist_id, is_primary, number) VALUES (?, ?, 1, 0)",
                        -1, &st, nullptr);
                    sqlite3_bind_text(st, 1, releaseKey.c_str(),       -1, SQLITE_STATIC);
                    sqlite3_bind_text(st, 2, primaryArtistKey.c_str(), -1, SQLITE_STATIC);
                    sqlite3_step(st);
                    sqlite3_finalize(st);
                }
            }
        }
    }
    if (releaseKey.empty()) return false;

    // ── Track ────────────────────────────────────────────────────────────────
    sqlite3_int64 trackId = -1;
    {
        sqlite3_stmt* st = nullptr;
        sqlite3_prepare_v2(db,
            "SELECT id FROM tracks WHERE recording_id = ? AND release_id = ?",
            -1, &st, nullptr);
        sqlite3_bind_text(st, 1, isrc.c_str(),       -1, SQLITE_STATIC);
        sqlite3_bind_text(st, 2, releaseKey.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(st) == SQLITE_ROW) trackId = sqlite3_column_int64(st, 0);
        sqlite3_finalize(st);
    }
    if (trackId < 0) {
        sqlite3_stmt* st = nullptr;
        sqlite3_prepare_v2(db,
            "INSERT INTO tracks (recording_id, release_id) VALUES (?, ?)",
            -1, &st, nullptr);
        sqlite3_bind_text(st, 1, isrc.c_str(),       -1, SQLITE_STATIC);
        sqlite3_bind_text(st, 2, releaseKey.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(st);
        trackId = sqlite3_last_insert_rowid(db);
        sqlite3_finalize(st);
    }

    // ── Junction: track ↔ audio_file ─────────────────────────────────────────
    {
        sqlite3_stmt* st = nullptr;
        sqlite3_prepare_v2(db,
            "INSERT OR IGNORE INTO track_audio_files (track_id, audio_file_id) "
            "VALUES (?, ?)",
            -1, &st, nullptr);
        sqlite3_bind_int64(st, 1, trackId);
        sqlite3_bind_int64(st, 2, audioFileId);
        sqlite3_step(st);
        sqlite3_finalize(st);
    }

    // ── Junction: playable ↔ track ────────────────────────────────────────────
    if (playableId >= 0) {
        sqlite3_stmt* st = nullptr;
        sqlite3_prepare_v2(db,
            "INSERT OR IGNORE INTO playable_tracks (playable_id, track_id) "
            "VALUES (?, ?)",
            -1, &st, nullptr);
        sqlite3_bind_int64(st, 1, playableId);
        sqlite3_bind_int64(st, 2, trackId);
        sqlite3_step(st);
        sqlite3_finalize(st);
    }

    addSave(db, trackId);
    return true;
}

// ── Progress helpers ──────────────────────────────────────────────────────────
// EVT_SCAN_PROGRESS payload:
//   GetInt()       → current index (1-based); 0 = collection phase (pulse)
//   GetExtraLong() → total file count
//   GetString()    → current file path
//   GetClientData()→ (intptr_t) imported count so far

void MusicScannerThread::postProgress(int current, int total,
                                       const std::string& file, int imported) {
    auto* e = new wxCommandEvent(EVT_SCAN_PROGRESS);
    e->SetInt(current);
    e->SetExtraLong((long)total);
    e->SetString(wxString::FromUTF8(file));
    e->SetClientData(reinterpret_cast<void*>((intptr_t)imported));
    wxQueueEvent(m_dialog, e);
}

// EVT_SCAN_DONE payload:
//   GetInt()       → total files scanned
//   GetExtraLong() → total files linked/imported

void MusicScannerThread::postDone(int total, int imported) {
    auto* e = new wxCommandEvent(EVT_SCAN_DONE);
    e->SetInt(total);
    e->SetExtraLong((long)imported);
    wxQueueEvent(m_dialog, e);
}

// ── Thread entry ──────────────────────────────────────────────────────────────

MusicScannerThread::MusicScannerThread(MusicScanDialog*    dlg,
                                        const std::string& rootPath,
                                        const std::string& dbPath)
    : wxThread(wxTHREAD_JOINABLE)
    , m_dialog(dlg)
    , m_rootPath(rootPath)
    , m_dbPath(dbPath)
{}

wxThread::ExitCode MusicScannerThread::Entry() {
    // ── Phase 1: collect audio file paths ────────────────────────────────────
    std::vector<std::string> files;
    {
        std::error_code ec;
        fs::directory_options opts = fs::directory_options::skip_permission_denied;
        auto it  = fs::recursive_directory_iterator(m_rootPath, opts, ec);
        auto end = fs::recursive_directory_iterator();
        for (; it != end && !TestDestroy(); it.increment(ec)) {
            if (ec) { ec.clear(); continue; }
            try {
                if (!it->is_regular_file()) continue;
                if (isAudioFile(it->path().extension().string()))
                    files.push_back(it->path().string());
            } catch (...) {}

            // Pulse the progress bar every 50 files during collection
            if (files.size() % 50 == 0)
                postProgress(0, 0, it->path().string(), 0);
        }
    }
    if (TestDestroy()) { postDone(0, 0); return nullptr; }

    int total    = (int)files.size();
    int imported = 0;

    // ── Open a thread-local SQLite connection ────────────────────────────────
    sqlite3* db = nullptr;
    if (sqlite3_open(m_dbPath.c_str(), &db) != SQLITE_OK) {
        sqlite3_close(db);
        postDone(total, 0);
        return nullptr;
    }
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;",    nullptr, nullptr, nullptr);
    sqlite3_exec(db, "PRAGMA journal_mode = WAL;",   nullptr, nullptr, nullptr);
    sqlite3_busy_timeout(db, 5000); // yield to other writers up to 5 s

    // ── Phase 2: process each file ───────────────────────────────────────────
    for (int i = 0; i < total && !TestDestroy(); ++i) {
        const std::string& path = files[i];
        postProgress(i + 1, total, path, imported);

        try {
            // Embedded tags (ID3, Vorbis comments, MP4 atoms) are broken out
            // into recording, artists, release and track rows by AudioImporter.
            // A file with no tags at all is first offered to MusicBrainz, which
            // may identify it from the "Artist - Title" in its filename.
            const space::AudioFileMetadata tags = space::AudioImporter::readMetadata(path);
            bool linked = false;
            if (!tags.hasTags && !tags.title.empty() && !tags.artists.empty()) {
                linked = isLinked(db, path); // identified on an earlier scan
                if (!linked)
                    linked = identifyWithMusicBrainz(db, path, tags);
            }
            if (linked || space::AudioImporter::importFile(db, path).fileId >= 0)
                ++imported;

        } catch (...) {
            // Per-file errors should not abort the whole scan
        }
    }

    sqlite3_close(db);
    postDone(total, imported);
    return nullptr;
}

// =============================================================================
// MusicScanDialog
// =============================================================================

MusicScanDialog::MusicScanDialog(wxWindow*          parent,
                                  const std::string& folderPath,
                                  const std::string& dbPath)
    : wxDialog(parent, wxID_ANY, "Scanning Music Library",
               wxDefaultPosition, wxSize(540, -1),
               wxDEFAULT_DIALOG_STYLE | wxSTAY_ON_TOP)
{
    Bind(EVT_SCAN_PROGRESS, &MusicScanDialog::OnProgress, this);
    Bind(EVT_SCAN_DONE,     &MusicScanDialog::OnDone,     this);

    auto* outer = new wxBoxSizer(wxVERTICAL);

    // Folder path label
    m_folderLabel = new wxStaticText(this, wxID_ANY,
        wxString::FromUTF8("Folder: " + folderPath),
        wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
    outer->Add(m_folderLabel, 0, wxEXPAND | wxALL, 12);

    // Current file label
    m_fileLabel = new wxStaticText(this, wxID_ANY, "Collecting files\u2026",
        wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_START);
    outer->Add(m_fileLabel, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
    outer->AddSpacer(4);

    // Progress gauge
    m_gauge = new wxGauge(this, wxID_ANY, 100,
        wxDefaultPosition, wxSize(-1, 18), wxGA_HORIZONTAL | wxGA_SMOOTH);
    outer->Add(m_gauge, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Stats label
    m_statsLabel = new wxStaticText(this, wxID_ANY, "0 / 0 files \u2013 0 imported");
    outer->Add(m_statsLabel, 0, wxALIGN_CENTER | wxBOTTOM, 8);

    // Cancel / Close button
    m_actionBtn = new wxButton(this, wxID_ANY, "Cancel");
    m_actionBtn->Bind(wxEVT_BUTTON, &MusicScanDialog::OnActionBtn, this);
    outer->Add(m_actionBtn, 0, wxALIGN_CENTER | wxBOTTOM, 12);

    Bind(wxEVT_CLOSE_WINDOW, &MusicScanDialog::OnClose, this);

    SetSizerAndFit(outer);
    CenterOnParent();

    // Launch the worker thread
    m_thread = new MusicScannerThread(this, folderPath, dbPath);
    if (m_thread->Create() != wxTHREAD_NO_ERROR
     || m_thread->Run()    != wxTHREAD_NO_ERROR) {
        delete m_thread;
        m_thread = nullptr;
        wxMessageBox("Failed to start the scan thread.", "Error",
                     wxOK | wxICON_ERROR, this);
        EndModal(wxID_CANCEL);
    }
}

MusicScanDialog::~MusicScanDialog() {
    // Safety net: if dialog is destroyed while thread is still running
    if (m_thread) {
        m_thread->Delete();
        delete m_thread;
        m_thread = nullptr;
    }
}

void MusicScanDialog::OnProgress(wxCommandEvent& e) {
    int current  = e.GetInt();
    int total    = (int)e.GetExtraLong();
    int imported = (int)reinterpret_cast<intptr_t>(e.GetClientData());

    if (total > 0 && current > 0) {
        m_gauge->SetRange(total);
        m_gauge->SetValue(current);
    } else {
        m_gauge->Pulse();
    }

    std::string filename = fs::path(e.GetString().ToStdString()).filename().string();
    m_fileLabel->SetLabel(wxString::FromUTF8("Scanning: " + filename));

    m_statsLabel->SetLabel(wxString::Format(
        "%d / %d files \u2013 %d imported", current, total, imported));
}

void MusicScanDialog::OnDone(wxCommandEvent& e) {
    int total    = e.GetInt();
    int imported = (int)e.GetExtraLong();

    // Reap the thread (should return instantly since Entry() just finished)
    if (m_thread) {
        m_thread->Wait();
        delete m_thread;
        m_thread = nullptr;
    }
    m_scanDone = true;

    m_gauge->SetRange(total > 0 ? total : 1);
    m_gauge->SetValue(total > 0 ? total : 1);
    m_fileLabel->SetLabel("Scan complete.");
    m_statsLabel->SetLabel(wxString::Format(
        "%d files scanned \u2013 %d imported / linked", total, imported));
    m_actionBtn->SetLabel("Close");
}

void MusicScanDialog::OnActionBtn(wxCommandEvent&) {
    if (m_scanDone) {
        EndModal(wxID_OK);
        return;
    }
    // Cancel: request thread to stop and wait (≤ 3 s for curl timeout)
    if (m_thread) {
        m_actionBtn->Enable(false);
        m_fileLabel->SetLabel("Cancelling\u2026");
        m_thread->Delete();
        delete m_thread;
        m_thread = nullptr;
    }
    EndModal(wxID_CANCEL);
}

void MusicScanDialog::OnClose(wxCloseEvent&) {
    wxCommandEvent dummy;
    OnActionBtn(dummy);
}
