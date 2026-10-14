#include "Database.h"

#include <utility>

// ── Construction / destruction ────────────────────────────────────────────────

Database::Database(const std::string& path) {
    if (sqlite3_open(path.c_str(), &m_db) != SQLITE_OK) {
        std::string err = sqlite3_errmsg(m_db);
        sqlite3_close(m_db);
        m_db = nullptr;
        throw std::runtime_error("Failed to open database '" + path + "': " + err);
    }
    exec("PRAGMA foreign_keys = ON;");
    initialize();
}

Database::~Database() {
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

Database::Database(Database&& other) noexcept : m_db(other.m_db) {
    other.m_db = nullptr;
}

Database& Database::operator=(Database&& other) noexcept {
    if (this != &other) {
        if (m_db) sqlite3_close(m_db);
        m_db = other.m_db;
        other.m_db = nullptr;
    }
    return *this;
}

// ── Private helpers ───────────────────────────────────────────────────────────

void Database::exec(const char* sql) {
    char* errmsg = nullptr;
    if (sqlite3_exec(m_db, sql, nullptr, nullptr, &errmsg) != SQLITE_OK) {
        std::string err(errmsg);
        sqlite3_free(errmsg);
        throw std::runtime_error("SQLite error: " + err);
    }
}

// ── Schema ────────────────────────────────────────────────────────────────────

void Database::initialize() {
    // Artists — must exist before recordings reference them
    exec(R"(
        CREATE TABLE IF NOT EXISTS artists (
            isni TEXT PRIMARY KEY,
            name TEXT NOT NULL
        );
    )");

    // Recordings
    exec(R"(
        CREATE TABLE IF NOT EXISTS recordings (
            isrc    TEXT PRIMARY KEY,
            name    TEXT NOT NULL,
            version TEXT
        );
    )");

    // Junction: recordings ↔ artists
    exec(R"(
        CREATE TABLE IF NOT EXISTS recording_artists (
            recording_id TEXT    NOT NULL REFERENCES recordings(isrc) ON DELETE CASCADE,
            artist_id    TEXT    NOT NULL REFERENCES artists(isni)    ON DELETE CASCADE,
            is_primary   INTEGER NOT NULL DEFAULT 0,
            number       INTEGER NOT NULL DEFAULT 0,
            PRIMARY KEY (recording_id, artist_id)
        );
    )");

    // Labels
    exec(R"(
        CREATE TABLE IF NOT EXISTS labels (
            id   INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT    NOT NULL
        );
    )");

    // Releases
    exec(R"(
        CREATE TABLE IF NOT EXISTS releases (
            upc          TEXT    PRIMARY KEY,
            name         TEXT    NOT NULL,
            release_date TEXT,
            label_id     INTEGER REFERENCES labels(id) ON DELETE SET NULL,
            cover_art    TEXT
        );
    )");

    // Tracks — bind a recording to a release
    exec(R"(
        CREATE TABLE IF NOT EXISTS tracks (
            id           INTEGER PRIMARY KEY AUTOINCREMENT,
            recording_id TEXT    NOT NULL REFERENCES recordings(isrc)  ON DELETE CASCADE,
            release_id   TEXT    NOT NULL REFERENCES releases(upc)     ON DELETE CASCADE
        );
    )");

    // Junction: releases ↔ artists
    exec(R"(
        CREATE TABLE IF NOT EXISTS release_artists (
            release_id TEXT    NOT NULL REFERENCES releases(upc)  ON DELETE CASCADE,
            artist_id  TEXT    NOT NULL REFERENCES artists(isni)  ON DELETE CASCADE,
            number     INTEGER NOT NULL DEFAULT 0,
            is_primary INTEGER NOT NULL DEFAULT 0,
            PRIMARY KEY (release_id, artist_id)
        );
    )");

    // Saves — soft-deletable references to a track
    exec(R"(
        CREATE TABLE IF NOT EXISTS saves (
            id         INTEGER PRIMARY KEY AUTOINCREMENT,
            track_id   INTEGER NOT NULL REFERENCES tracks(id) ON DELETE CASCADE,
            saved_at   TEXT    NOT NULL DEFAULT (datetime('now')),
            deleted_at TEXT
        );
    )");

    // Files — paths to local or remote media files
    exec(R"(
        CREATE TABLE IF NOT EXISTS files (
            id   INTEGER PRIMARY KEY AUTOINCREMENT,
            path TEXT    NOT NULL UNIQUE
        );
    )");

    // Audio files — embedded/resolved metadata for a local file
    exec(R"(
        CREATE TABLE IF NOT EXISTS audio_files (
            id        INTEGER PRIMARY KEY AUTOINCREMENT,
            file_id   INTEGER NOT NULL REFERENCES files(id) ON DELETE CASCADE,
            name      TEXT,
            version   TEXT,
            artists   TEXT,
            album     TEXT,
            duration  INTEGER,
            cover_art TEXT
        );
    )");
    // Migration: add columns introduced after initial schema (ignored if they exist)
    sqlite3_exec(m_db, "ALTER TABLE audio_files ADD COLUMN version TEXT",    nullptr, nullptr, nullptr);
    sqlite3_exec(m_db, "ALTER TABLE audio_files ADD COLUMN cover_art TEXT",  nullptr, nullptr, nullptr);

    // Junction: tracks ↔ audio_files
    exec(R"(
        CREATE TABLE IF NOT EXISTS track_audio_files (
            track_id      INTEGER NOT NULL REFERENCES tracks(id)      ON DELETE CASCADE,
            audio_file_id INTEGER NOT NULL REFERENCES audio_files(id) ON DELETE CASCADE,
            PRIMARY KEY (track_id, audio_file_id)
        );
    )");

    // Playables — a URI that can be played directly
    exec(R"(
        CREATE TABLE IF NOT EXISTS playables (
            id  INTEGER PRIMARY KEY AUTOINCREMENT,
            uri TEXT    NOT NULL UNIQUE
        );
    )");

    // Junction: playables ↔ tracks
    exec(R"(
        CREATE TABLE IF NOT EXISTS playable_tracks (
            playable_id INTEGER NOT NULL REFERENCES playables(id) ON DELETE CASCADE,
            track_id    INTEGER NOT NULL REFERENCES tracks(id)    ON DELETE CASCADE,
            PRIMARY KEY (playable_id, track_id)
        );
    )");

    // Downloads — tracks file downloads from the web browser
    exec(R"(
        CREATE TABLE IF NOT EXISTS downloads (
            id           INTEGER PRIMARY KEY AUTOINCREMENT,
            url          TEXT    NOT NULL,
            filename     TEXT    NOT NULL,
            file_path    TEXT,
            content_type TEXT,
            status       TEXT    NOT NULL DEFAULT 'ongoing',
            total_bytes  INTEGER NOT NULL DEFAULT 0,
            done_bytes   INTEGER NOT NULL DEFAULT 0,
            created_at   TEXT    NOT NULL DEFAULT (datetime('now')),
            completed_at TEXT
        );
    )");

    // Bookmarks — user-saved URIs displayed in the left sidebar
    exec(R"(
        CREATE TABLE IF NOT EXISTS bookmarks (
            id         INTEGER PRIMARY KEY AUTOINCREMENT,
            uri        TEXT    NOT NULL UNIQUE,
            title      TEXT    NOT NULL,
            parent_id  INTEGER REFERENCES bookmarks(id) ON DELETE CASCADE,
            created_at TEXT    NOT NULL DEFAULT (datetime('now'))
        );
    )");

    // Accepted third-party domains — hostnames the user has acknowledged the warning for
    exec(R"(
        CREATE TABLE IF NOT EXISTS accepted_domains (
            hostname TEXT NOT NULL UNIQUE
        );
    )");

    // Migration: make tracks.release_id nullable (allows tracks without a release).
    // Detects the NOT NULL constraint via PRAGMA table_info and rebuilds the table if needed.
    {
        bool needsMigration = false;
        sqlite3_stmt* chk = nullptr;
        if (sqlite3_prepare_v2(m_db, "PRAGMA table_info(tracks);", -1, &chk, nullptr) == SQLITE_OK) {
            while (sqlite3_step(chk) == SQLITE_ROW) {
                const char* col = reinterpret_cast<const char*>(sqlite3_column_text(chk, 1));
                if (col && std::string(col) == "release_id" && sqlite3_column_int(chk, 3) != 0) {
                    needsMigration = true;
                }
            }
            sqlite3_finalize(chk);
        }
        if (needsMigration) {
            sqlite3_exec(m_db, "PRAGMA foreign_keys = OFF;", nullptr, nullptr, nullptr);
            sqlite3_exec(m_db, "BEGIN;", nullptr, nullptr, nullptr);
            bool ok = true;
            ok = ok && (sqlite3_exec(m_db, "DROP TABLE IF EXISTS tracks_new;",
                                     nullptr, nullptr, nullptr) == SQLITE_OK);
            ok = ok && (sqlite3_exec(m_db, R"(
                CREATE TABLE tracks_new (
                    id           INTEGER PRIMARY KEY AUTOINCREMENT,
                    recording_id TEXT    NOT NULL REFERENCES recordings(isrc) ON DELETE CASCADE,
                    release_id   TEXT    REFERENCES releases(upc) ON DELETE SET NULL
                );
            )", nullptr, nullptr, nullptr) == SQLITE_OK);
            ok = ok && (sqlite3_exec(m_db,
                "INSERT OR IGNORE INTO tracks_new SELECT id, recording_id, release_id FROM tracks;",
                nullptr, nullptr, nullptr) == SQLITE_OK);
            ok = ok && (sqlite3_exec(m_db, "DROP TABLE tracks;",
                                     nullptr, nullptr, nullptr) == SQLITE_OK);
            ok = ok && (sqlite3_exec(m_db, "ALTER TABLE tracks_new RENAME TO tracks;",
                                     nullptr, nullptr, nullptr) == SQLITE_OK);
            sqlite3_exec(m_db, ok ? "COMMIT;" : "ROLLBACK;", nullptr, nullptr, nullptr);
            sqlite3_exec(m_db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
        }
    }
}

// ── Bookmark CRUD ─────────────────────────────────────────────────────────────

void Database::addBookmark(const std::string& uri, const std::string& title, int parentId)
{
    const char* sql = parentId == -1
        ? "INSERT OR IGNORE INTO bookmarks (uri, title) VALUES (?, ?);"
        : "INSERT OR IGNORE INTO bookmarks (uri, title, parent_id) VALUES (?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text(stmt, 1, uri.c_str(),   -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_TRANSIENT);
    if (parentId != -1)
        sqlite3_bind_int(stmt, 3, parentId);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void Database::removeBookmark(const std::string& uri)
{
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, "DELETE FROM bookmarks WHERE uri = ?;", -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text(stmt, 1, uri.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

bool Database::isBookmarked(const std::string& uri)
{
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, "SELECT 1 FROM bookmarks WHERE uri = ? LIMIT 1;", -1, &stmt, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_text(stmt, 1, uri.c_str(), -1, SQLITE_TRANSIENT);
    bool found = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return found;
}

int Database::getBookmarkId(const std::string& uri)
{
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, "SELECT id FROM bookmarks WHERE uri = ? LIMIT 1;", -1, &stmt, nullptr) != SQLITE_OK)
        return -1;
    sqlite3_bind_text(stmt, 1, uri.c_str(), -1, SQLITE_TRANSIENT);
    int id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        id = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return id;
}

std::vector<Database::Bookmark> Database::getTopLevelBookmarks(const std::string& uriPrefix)
{
    std::vector<Bookmark> result;
    const std::string pattern = uriPrefix + "%";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "SELECT id, uri, title FROM bookmarks WHERE uri LIKE ? AND parent_id IS NULL ORDER BY created_at;",
        -1, &stmt, nullptr) != SQLITE_OK) return result;
    sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Bookmark b;
        b.id    = sqlite3_column_int(stmt, 0);
        b.uri   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        b.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        result.push_back(b);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<Database::Bookmark> Database::getChildBookmarks(int parentId)
{
    std::vector<Bookmark> result;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "SELECT id, uri, title FROM bookmarks WHERE parent_id = ? ORDER BY created_at;",
        -1, &stmt, nullptr) != SQLITE_OK) return result;
    sqlite3_bind_int(stmt, 1, parentId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Bookmark b;
        b.id       = sqlite3_column_int(stmt, 0);
        b.uri      = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        b.title    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        b.parentId = parentId;
        result.push_back(b);
    }
    sqlite3_finalize(stmt);
    return result;
}

// ── Accepted third-party domains ──────────────────────────────────────────────

bool Database::isDomainAccepted(const std::string& hostname)
{
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "SELECT 1 FROM accepted_domains WHERE hostname = ? LIMIT 1;",
        -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, hostname.c_str(), -1, SQLITE_TRANSIENT);
    bool found = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return found;
}

void Database::addAcceptedDomain(const std::string& hostname)
{
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "INSERT OR IGNORE INTO accepted_domains (hostname) VALUES (?);",
        -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text(stmt, 1, hostname.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

// ── Music metadata CRUD ───────────────────────────────────────────────────────

// Internal helper: bind optional text (empty string → SQL NULL)
static void bindOptText(sqlite3_stmt* s, int col, const std::string& v) {
    if (v.empty())
        sqlite3_bind_null(s, col);
    else
        sqlite3_bind_text(s, col, v.c_str(), -1, SQLITE_TRANSIENT);
}

// Internal helper: column text as std::string (returns "" for NULL)
static std::string colText(sqlite3_stmt* s, int col) {
    const unsigned char* t = sqlite3_column_text(s, col);
    return t ? reinterpret_cast<const char*>(t) : "";
}

std::vector<Database::ArtistRecord> Database::getAllArtists()
{
    std::vector<ArtistRecord> out;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "SELECT isni, name FROM artists ORDER BY name;",
        -1, &stmt, nullptr) != SQLITE_OK) return out;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        out.push_back({ colText(stmt, 0), colText(stmt, 1) });
    sqlite3_finalize(stmt);
    return out;
}

std::vector<Database::RecordingRecord> Database::getAllRecordings()
{
    std::vector<RecordingRecord> out;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "SELECT isrc, name, COALESCE(version,'') FROM recordings ORDER BY name;",
        -1, &stmt, nullptr) != SQLITE_OK) return out;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        out.push_back({ colText(stmt, 0), colText(stmt, 1), colText(stmt, 2) });
    sqlite3_finalize(stmt);
    return out;
}

std::vector<Database::ReleaseRecord> Database::getAllReleases()
{
    std::vector<ReleaseRecord> out;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "SELECT upc, name, COALESCE(release_date,'') FROM releases ORDER BY name;",
        -1, &stmt, nullptr) != SQLITE_OK) return out;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        out.push_back({ colText(stmt, 0), colText(stmt, 1), colText(stmt, 2) });
    sqlite3_finalize(stmt);
    return out;
}

void Database::upsertArtist(const std::string& isni, const std::string& name)
{
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "INSERT INTO artists (isni, name) VALUES (?, ?)"
        "  ON CONFLICT(isni) DO UPDATE SET name = excluded.name;",
        -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text(stmt, 1, isni.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void Database::upsertRecording(const std::string& isrc, const std::string& name,
                                const std::string& version)
{
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "INSERT INTO recordings (isrc, name, version) VALUES (?, ?, ?)"
        "  ON CONFLICT(isrc) DO UPDATE SET name = excluded.name, version = excluded.version;",
        -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text(stmt, 1, isrc.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, name.c_str(),    -1, SQLITE_TRANSIENT);
    bindOptText(stmt, 3, version);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void Database::upsertRelease(const std::string& upc, const std::string& name,
                              const std::string& releaseDate)
{
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "INSERT INTO releases (upc, name, release_date) VALUES (?, ?, ?)"
        "  ON CONFLICT(upc) DO UPDATE SET name = excluded.name,"
        "    release_date = excluded.release_date;",
        -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text(stmt, 1, upc.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    bindOptText(stmt, 3, releaseDate);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

int64_t Database::upsertTrack(const std::string& recordingIsrc, const std::string& releaseUpc)
{
    // Look for an existing track with the same (recording_id, release_id) pair.
    sqlite3_stmt* sel = nullptr;
    const char* selectSql = releaseUpc.empty()
        ? "SELECT id FROM tracks WHERE recording_id = ? AND release_id IS NULL LIMIT 1;"
        : "SELECT id FROM tracks WHERE recording_id = ? AND release_id = ? LIMIT 1;";
    if (sqlite3_prepare_v2(m_db, selectSql, -1, &sel, nullptr) != SQLITE_OK) return -1;
    sqlite3_bind_text(sel, 1, recordingIsrc.c_str(), -1, SQLITE_TRANSIENT);
    if (!releaseUpc.empty())
        sqlite3_bind_text(sel, 2, releaseUpc.c_str(), -1, SQLITE_TRANSIENT);
    int64_t existingId = -1;
    if (sqlite3_step(sel) == SQLITE_ROW)
        existingId = sqlite3_column_int64(sel, 0);
    sqlite3_finalize(sel);
    if (existingId != -1) return existingId;

    // Insert new track
    sqlite3_stmt* ins = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "INSERT INTO tracks (recording_id, release_id) VALUES (?, ?);",
        -1, &ins, nullptr) != SQLITE_OK) return -1;
    sqlite3_bind_text(ins, 1, recordingIsrc.c_str(), -1, SQLITE_TRANSIENT);
    bindOptText(ins, 2, releaseUpc);
    sqlite3_step(ins);
    int64_t newId = sqlite3_last_insert_rowid(m_db);
    sqlite3_finalize(ins);
    return newId;
}

void Database::setRecordingArtists(const std::string& recordingIsrc,
                                    const std::vector<ArtistRelationship>& artists)
{
    // Delete existing relationships then re-insert
    sqlite3_stmt* del = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "DELETE FROM recording_artists WHERE recording_id = ?;",
        -1, &del, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(del, 1, recordingIsrc.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(del);
        sqlite3_finalize(del);
    }
    sqlite3_stmt* ins = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "INSERT INTO recording_artists (recording_id, artist_id, is_primary, number)"
        "  VALUES (?, ?, ?, ?);",
        -1, &ins, nullptr) != SQLITE_OK) return;
    for (const auto& a : artists) {
        sqlite3_reset(ins);
        sqlite3_bind_text(ins, 1, recordingIsrc.c_str(),  -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(ins, 2, a.artistIsni.c_str(),   -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(ins,  3, a.isPrimary ? 1 : 0);
        sqlite3_bind_int(ins,  4, a.number);
        sqlite3_step(ins);
    }
    sqlite3_finalize(ins);
}

void Database::setReleaseArtists(const std::string& releaseUpc,
                                  const std::vector<ArtistRelationship>& artists)
{
    sqlite3_stmt* del = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "DELETE FROM release_artists WHERE release_id = ?;",
        -1, &del, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(del, 1, releaseUpc.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(del);
        sqlite3_finalize(del);
    }
    sqlite3_stmt* ins = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "INSERT INTO release_artists (release_id, artist_id, number, is_primary)"
        "  VALUES (?, ?, ?, ?);",
        -1, &ins, nullptr) != SQLITE_OK) return;
    for (const auto& a : artists) {
        sqlite3_reset(ins);
        sqlite3_bind_text(ins, 1, releaseUpc.c_str(),    -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(ins, 2, a.artistIsni.c_str(),  -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(ins,  3, a.number);
        sqlite3_bind_int(ins,  4, a.isPrimary ? 1 : 0);
        sqlite3_step(ins);
    }
    sqlite3_finalize(ins);
}

int64_t Database::ensureSave(int64_t trackId)
{
    // Check for existing active save
    {
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(m_db,
            "SELECT id FROM saves WHERE track_id = ? AND deleted_at IS NULL LIMIT 1;",
            -1, &s, nullptr) == SQLITE_OK) {
            sqlite3_bind_int64(s, 1, trackId);
            if (sqlite3_step(s) == SQLITE_ROW) {
                int64_t id = sqlite3_column_int64(s, 0);
                sqlite3_finalize(s);
                return id;
            }
            sqlite3_finalize(s);
        }
    }
    // Reactivate soft-deleted save if one exists
    {
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(m_db,
            "SELECT id FROM saves WHERE track_id = ? AND deleted_at IS NOT NULL LIMIT 1;",
            -1, &s, nullptr) == SQLITE_OK) {
            sqlite3_bind_int64(s, 1, trackId);
            if (sqlite3_step(s) == SQLITE_ROW) {
                int64_t id = sqlite3_column_int64(s, 0);
                sqlite3_finalize(s);
                sqlite3_stmt* upd = nullptr;
                if (sqlite3_prepare_v2(m_db,
                    "UPDATE saves SET deleted_at = NULL, saved_at = datetime('now') WHERE id = ?;",
                    -1, &upd, nullptr) == SQLITE_OK) {
                    sqlite3_bind_int64(upd, 1, id);
                    sqlite3_step(upd);
                    sqlite3_finalize(upd);
                }
                return id;
            }
            sqlite3_finalize(s);
        }
    }
    // Insert new save
    sqlite3_stmt* ins = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "INSERT INTO saves (track_id) VALUES (?);",
        -1, &ins, nullptr) != SQLITE_OK) return -1;
    sqlite3_bind_int64(ins, 1, trackId);
    sqlite3_step(ins);
    int64_t newId = sqlite3_last_insert_rowid(m_db);
    sqlite3_finalize(ins);
    return newId;
}

Database::TrackDetails Database::getTrackDetails(int trackId)
{
    TrackDetails out;
    out.id = trackId;

    // Recording + release
    {
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(m_db, R"(
            SELECT rec.isrc, rec.name, COALESCE(rec.version,''),
                   COALESCE(rel.upc,''), COALESCE(rel.name,''),
                   COALESCE(rel.release_date,'')
            FROM tracks t
            JOIN recordings rec ON rec.isrc = t.recording_id
            LEFT JOIN releases rel ON rel.upc = t.release_id
            WHERE t.id = ?;
        )", -1, &s, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(s, 1, trackId);
            if (sqlite3_step(s) == SQLITE_ROW) {
                out.recording = { colText(s,0), colText(s,1), colText(s,2) };
                out.release   = { colText(s,3), colText(s,4), colText(s,5) };
            }
            sqlite3_finalize(s);
        }
    }

    // Recording artists
    {
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(m_db, R"(
            SELECT a.isni, a.name, ra.is_primary, ra.number
            FROM recording_artists ra
            JOIN artists a ON a.isni = ra.artist_id
            WHERE ra.recording_id = ?
            ORDER BY ra.number;
        )", -1, &s, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(s, 1, out.recording.isrc.c_str(), -1, SQLITE_TRANSIENT);
            while (sqlite3_step(s) == SQLITE_ROW) {
                out.recordingArtists.push_back({
                    colText(s,0), colText(s,1),
                    sqlite3_column_int(s,2) != 0,
                    sqlite3_column_int(s,3)
                });
            }
            sqlite3_finalize(s);
        }
    }

    // Release artists
    if (!out.release.upc.empty()) {
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(m_db, R"(
            SELECT a.isni, a.name, ra.is_primary, ra.number
            FROM release_artists ra
            JOIN artists a ON a.isni = ra.artist_id
            WHERE ra.release_id = ?
            ORDER BY ra.number;
        )", -1, &s, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(s, 1, out.release.upc.c_str(), -1, SQLITE_TRANSIENT);
            while (sqlite3_step(s) == SQLITE_ROW) {
                out.releaseArtists.push_back({
                    colText(s,0), colText(s,1),
                    sqlite3_column_int(s,2) != 0,
                    sqlite3_column_int(s,3)
                });
            }
            sqlite3_finalize(s);
        }
    }

    return out;
}
