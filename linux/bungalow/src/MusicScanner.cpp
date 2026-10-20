#include "MusicScanner.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/msgdlg.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

#include <boost/json/parse.hpp>
#include <boost/json/value.hpp>

#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>
#include <regex>
#include <cstdio>
#include <cstring>

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
static const char* kTaggableExts[] = { // have ID3v1/v2 tags
    ".mp3", ".aac", ".wav", nullptr
};

bool MusicScannerThread::isAudioFile(const std::string& ext) {
    std::string lo = ext;
    std::transform(lo.begin(), lo.end(), lo.begin(), ::tolower);
    for (int i = 0; kAudioExts[i]; ++i)
        if (lo == kAudioExts[i]) return true;
    return false;
}

bool MusicScannerThread::isTaggableAudio(const std::string& ext) {
    std::string lo = ext;
    std::transform(lo.begin(), lo.end(), lo.begin(), ::tolower);
    for (int i = 0; kTaggableExts[i]; ++i)
        if (lo == kTaggableExts[i]) return true;
    return false;
}

// ── ID3v2 binary helpers ──────────────────────────────────────────────────────

static uint32_t readBE32(const uint8_t* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16)
         | (uint32_t(p[2]) <<  8) |  uint32_t(p[3]);
}

static uint32_t readSyncsafe4(const uint8_t* p) {
    return (uint32_t(p[0] & 0x7F) << 21) | (uint32_t(p[1] & 0x7F) << 14)
         | (uint32_t(p[2] & 0x7F) <<  7) |  uint32_t(p[3] & 0x7F);
}

// Decode a text frame (first byte = encoding).
// Returns a UTF-8 string suitable for SQLite storage.
static std::string decodeTextFrame(const uint8_t* data, size_t size) {
    if (size == 0) return {};
    uint8_t enc = data[0];
    const uint8_t* text = data + 1;
    size_t len = size - 1;

    // Latin-1 (0) or UTF-8 (3): treat the bytes as UTF-8 (ASCII subset is compatible)
    if (enc == 0 || enc == 3) {
        std::string s(reinterpret_cast<const char*>(text), len);
        // strip embedded NULs
        s.erase(std::remove(s.begin(), s.end(), '\0'), s.end());
        return s;
    }

    // UTF-16 with BOM (1) or UTF-16BE without BOM (2)
    if (len < 2) return {};
    bool bigEndian = (enc == 2);
    size_t off = 0;
    if (len >= 2) {
        if (text[0] == 0xFF && text[1] == 0xFE) { bigEndian = false; off = 2; }
        else if (text[0] == 0xFE && text[1] == 0xFF) { bigEndian = true;  off = 2; }
    }
    std::string result;
    for (size_t i = off; i + 1 < len; i += 2) {
        uint16_t ch = bigEndian
            ? (uint16_t(text[i]) << 8 | text[i + 1])
            : (uint16_t(text[i + 1]) << 8 | text[i]);
        if (ch == 0) break;
        if (ch < 0x80) {
            result += char(ch);
        } else if (ch < 0x800) {
            result += char(0xC0 | (ch >> 6));
            result += char(0x80 | (ch & 0x3F));
        } else {
            result += char(0xE0 | (ch >> 12));
            result += char(0x80 | ((ch >> 6) & 0x3F));
            result += char(0x80 | (ch & 0x3F));
        }
    }
    return result;
}

// ── ID3v2 reader ──────────────────────────────────────────────────────────────

AudioMetadata MusicScannerThread::readID3v2(const std::string& path) {
    AudioMetadata meta;
    std::ifstream f(path, std::ios::binary);
    if (!f) return meta;

    uint8_t hdr[10];
    if (!f.read(reinterpret_cast<char*>(hdr), 10)) return meta;
    if (hdr[0] != 'I' || hdr[1] != 'D' || hdr[2] != '3') return meta;

    int ver = hdr[3]; // major version: 2, 3, or 4
    if (ver < 2 || ver > 4) return meta;

    uint32_t tagBytes = readSyncsafe4(hdr + 6);
    uint32_t consumed = 0;

    while (consumed + 10 < tagBytes && !TestDestroy()) {
        uint8_t fh[10];
        if (!f.read(reinterpret_cast<char*>(fh), 10)) break;
        if (fh[0] == 0) break; // padding

        std::string fid(reinterpret_cast<char*>(fh), 4);
        uint32_t fsize = (ver == 4) ? readSyncsafe4(fh + 4) : readBE32(fh + 4);
        // fh[8,9] are frame flags – ignored
        consumed += 10 + fsize;

        if (fsize == 0 || fsize > 2 * 1024 * 1024) break;

        std::vector<uint8_t> data(fsize);
        if (!f.read(reinterpret_cast<char*>(data.data()), fsize)) break;

        if      (fid == "TIT2") meta.title   = decodeTextFrame(data.data(), data.size());
        else if (fid == "TPE1") meta.artists = decodeTextFrame(data.data(), data.size());
        else if (fid == "TALB") meta.album   = decodeTextFrame(data.data(), data.size());
        else if (fid == "TLEN") {
            std::string s = decodeTextFrame(data.data(), data.size());
            try { meta.duration = std::stoi(s); } catch (...) {}
        }

        // Early-exit once we have all fields we care about
        if (!meta.title.empty() && !meta.artists.empty()
                && !meta.album.empty() && meta.duration > 0)
            break;
    }
    return meta;
}

// ── ID3v1 reader ──────────────────────────────────────────────────────────────

AudioMetadata MusicScannerThread::readID3v1(const std::string& path) {
    AudioMetadata meta;
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return meta;
    std::streampos sz = f.tellg();
    if (sz < 128) return meta;
    f.seekg(-128, std::ios::end);
    uint8_t tag[128];
    if (!f.read(reinterpret_cast<char*>(tag), 128)) return meta;
    if (tag[0] != 'T' || tag[1] != 'A' || tag[2] != 'G') return meta;

    auto trimField = [](const char* s, int len) {
        std::string r(s, strnlen(s, len));
        while (!r.empty() && (r.back() == ' ' || r.back() == '\0')) r.pop_back();
        return r;
    };
    meta.title   = trimField(reinterpret_cast<const char*>(tag +  3), 30);
    meta.artists = trimField(reinterpret_cast<const char*>(tag + 33), 30);
    meta.album   = trimField(reinterpret_cast<const char*>(tag + 63), 30);
    return meta;
}

// ── Tag dispatcher ────────────────────────────────────────────────────────────

static void trimString(std::string& s) {
    s.erase(std::remove(s.begin(), s.end(), '\0'), s.end());
    size_t b = s.find_first_not_of(" \t\r\n");
    size_t e = s.find_last_not_of(" \t\r\n");
    if (b == std::string::npos) s.clear();
    else s = s.substr(b, e - b + 1);
}

AudioMetadata MusicScannerThread::readTags(const std::string& path) {
    std::string ext = fs::path(path).extension().string();
    AudioMetadata meta;

    if (isTaggableAudio(ext)) {
        meta = readID3v2(path);
        if (meta.title.empty())
            meta = readID3v1(path);
    }
    // For MP4/M4A/FLAC/OGG we fall through to filename fallback
    // (those require separate atom/Vorbis parsers; MusicBrainz will fill gaps)

    // Filename fallback: parse "Artist - Title" from the stem for any fields
    // still missing from tags (e.g. "Daft Punk - Get Lucky (Radio Edit).mp3")
    {
        std::string stem = fs::path(path).stem().string();
        size_t sep = stem.find(" - ");
        if (sep != std::string::npos) {
            if (meta.artists.empty()) {
                meta.artists = stem.substr(0, sep);
                trimString(meta.artists);
            }
            if (meta.title.empty()) {
                meta.title = stem.substr(sep + 3);
                trimString(meta.title);
            }
        } else if (meta.title.empty()) {
            meta.title = stem;
        }
    }

    trimString(meta.title);
    trimString(meta.artists);
    trimString(meta.album);
    return meta;
}

// ── Version extraction ────────────────────────────────────────────────────────
// Strips a version suffix from the end of a title and returns it.
// Handles: "Title [Version]", "Title (Version)", "Title - Version"

std::string MusicScannerThread::extractVersion(std::string& title) {
    // Try bracket forms first (more specific), then dash
    struct Pattern { std::regex re; int captureGroup; };
    static const std::vector<Pattern> kPatterns = {
        { std::regex(R"(\s*\[([^\]]+)\]\s*$)"),               1 },
        { std::regex(R"(\s*\(([^)]+)\)\s*$)"),                1 },
        { std::regex(R"(\s*[-\u2013]\s*([^()\[\]]+?)\s*$)"), 1 },
    };
    for (auto& p : kPatterns) {
        std::smatch m;
        if (std::regex_search(title, m, p.re)) {
            std::string version = m[p.captureGroup].str();
            trimString(version);
            title = title.substr(0, m.position());
            trimString(title);
            return version;
        }
    }
    return {};
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

// Try to find a matching track in the local DB (by title + optional album) and
// insert a track_audio_files row for each match.
bool MusicScannerThread::matchAndLink(sqlite3* db, const AudioMetadata& meta,
                                       sqlite3_int64 audioFileId,
                                       sqlite3_int64 playableId) {
    if (meta.title.empty()) return false;

    // Match by recording name; also constrain on release name when an album tag
    // is present.  Returns all matching track ids (one recording may appear on
    // multiple releases so we link to all of them).
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(db, R"(
        SELECT DISTINCT t.id
        FROM tracks t
        JOIN recordings rec ON rec.isrc = t.recording_id
        JOIN releases   rel ON rel.upc  = t.release_id
        WHERE LOWER(rec.name) = LOWER(?)
          AND (? = '' OR LOWER(rel.name) = LOWER(?))
        LIMIT 10
    )", -1, &st, nullptr);
    sqlite3_bind_text(st, 1, meta.title.c_str(),  -1, SQLITE_STATIC);
    sqlite3_bind_text(st, 2, meta.album.c_str(),  -1, SQLITE_STATIC);
    sqlite3_bind_text(st, 3, meta.album.c_str(),  -1, SQLITE_STATIC);

    std::vector<sqlite3_int64> trackIds;
    while (sqlite3_step(st) == SQLITE_ROW)
        trackIds.push_back(sqlite3_column_int64(st, 0));
    sqlite3_finalize(st);

    if (trackIds.empty()) return false;

    for (sqlite3_int64 tid : trackIds) {
        sqlite3_prepare_v2(db,
            "INSERT OR IGNORE INTO track_audio_files (track_id, audio_file_id) VALUES (?, ?)",
            -1, &st, nullptr);
        sqlite3_bind_int64(st, 1, tid);
        sqlite3_bind_int64(st, 2, audioFileId);
        sqlite3_step(st);
        sqlite3_finalize(st);

        if (playableId >= 0) {
            sqlite3_prepare_v2(db,
                "INSERT OR IGNORE INTO playable_tracks (playable_id, track_id) VALUES (?, ?)",
                -1, &st, nullptr);
            sqlite3_bind_int64(st, 1, playableId);
            sqlite3_bind_int64(st, 2, tid);
            sqlite3_step(st);
            sqlite3_finalize(st);
        }

        addSave(db, tid);
    }
    return true;
}

// ── MusicBrainz lookup ────────────────────────────────────────────────────────

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
    boost::json::value jv;
    try { jv = boost::json::parse(jsonStr); } catch (...) { return false; }

    auto* root = jv.if_object();
    if (!root || !root->contains("recordings")) return false;
    auto* recs = root->at("recordings").if_array();
    if (!recs || recs->empty()) return false;

    auto* rec = (*recs)[0].if_object();
    if (!rec) return false;

    // Require a minimum confidence score
    if (rec->contains("score")) {
        try {
            int score = (int)rec->at("score").as_int64();
            if (score < 85) return false;
        } catch (...) {}
    }

    // ── Recording identity ──────────────────────────────────────────────────
    std::string mbid, recTitle;
    try {
        mbid     = std::string(rec->at("id").as_string().c_str());
        recTitle = std::string(rec->at("title").as_string().c_str());
    } catch (...) { return false; }

    // Prefer real ISRC; fall back to "mb:<mbid>"
    std::string isrc = "mb:" + mbid;
    if (rec->contains("isrcs")) {
        auto* isrcs = rec->at("isrcs").if_array();
        if (isrcs && !isrcs->empty()) {
            try { isrc = std::string((*isrcs)[0].as_string().c_str()); } catch (...) {}
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
        auto* credits = rec->at("artist-credit").if_array();
        if (credits) {
            int num = 0;
            for (auto& c : *credits) {
                auto* co = c.if_object();
                if (!co || !co->contains("artist")) continue;
                auto* a = co->at("artist").if_object();
                if (!a) continue;

                std::string aId, aName;
                try {
                    aId   = "mb:" + std::string(a->at("id").as_string().c_str());
                    aName = std::string(a->at("name").as_string().c_str());
                } catch (...) { continue; }

                // Prefer ISNI if present
                if (a->contains("isnis")) {
                    auto* isnis = a->at("isnis").if_array();
                    if (isnis && !isnis->empty()) {
                        try { aId = std::string((*isnis)[0].as_string().c_str()); }
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
        auto* rels = rec->at("releases").if_array();
        if (rels && !rels->empty()) {
            auto* rel = (*rels)[0].if_object();
            if (rel) {
                std::string relMbid, relTitle, relDate;
                try {
                    relMbid  = std::string(rel->at("id").as_string().c_str());
                    relTitle = std::string(rel->at("title").as_string().c_str());
                } catch (...) {}

                if (rel->contains("date") && !(*rel)["date"].is_null()) {
                    try { relDate = std::string(rel->at("date").as_string().c_str()); }
                    catch (...) {}
                }

                // Prefer UPC/barcode; fall back to "mb:<release-mbid>"
                releaseKey = "mb:" + relMbid;
                if (rel->contains("barcode") && !(*rel)["barcode"].is_null()) {
                    try {
                        std::string bc = std::string(rel->at("barcode").as_string().c_str());
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
            sqlite3_int64 fileId = upsertFile(db, path);
            if (fileId < 0) continue;

            AudioMetadata meta = readTags(path);
            meta.version       = extractVersion(meta.title);

            sqlite3_int64 afId = upsertAudioFile(db, fileId, meta);
            if (afId < 0) continue;

            ++imported; // count every file successfully stored in audio_files

            sqlite3_int64 playableId = upsertPlayable(db, path);

            bool linked = matchAndLink(db, meta, afId, playableId);

            if (!linked && !meta.title.empty() && !TestDestroy()) {
                // Throttle to ≤ 1 MusicBrainz request per second (ToS requirement)
                for (int ms = 0; ms < 1100 && !TestDestroy(); ms += 100)
                    wxThread::Sleep(100);

                if (!TestDestroy())
                    musicBrainzLookup(db, meta, afId, playableId);
            }

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
