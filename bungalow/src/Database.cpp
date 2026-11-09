#include "Database.h"

#include <algorithm>
#include <initializer_list>
#include <unordered_map>
#include <utility>
#include <random>
#include <sstream>
#include <iomanip>

// ── UUID4 generator ───────────────────────────────────────────────────────────

static std::string generateUUID4()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);

    uint32_t a = dist(gen);
    uint32_t b = dist(gen);
    uint32_t c = dist(gen);
    uint32_t d = dist(gen);

    // Version 4: bits 12-15 of b must be 0100
    b = (b & 0xFFFF0FFFu) | 0x00004000u;
    // Variant 10xx: bits 30-31 of c must be 10
    c = (c & 0x3FFFFFFFu) | 0x80000000u;

    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << std::setw(8) << a                    << '-'
        << std::setw(4) << ((b >> 16) & 0xFFFFu) << '-'
        << std::setw(4) << ( b        & 0xFFFFu) << '-'
        << std::setw(4) << ((c >> 16) & 0xFFFFu) << '-'
        << std::setw(4) << ( c        & 0xFFFFu)
        << std::setw(8) << d;
    return oss.str();
}


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
    initializeCustomTables();
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

    // "Play with…" picks — which music service plays a track (service::ResolutionStore).
    // Bound to the library track when there is one, and always to a stable key.
    exec(R"(
        CREATE TABLE IF NOT EXISTS music_service_tracks (
            id           INTEGER PRIMARY KEY AUTOINCREMENT,
            track_id     INTEGER REFERENCES tracks(id) ON DELETE CASCADE,
            track_key    TEXT    NOT NULL,
            service_id   TEXT    NOT NULL,
            match_isrc   TEXT,
            match_title  TEXT,
            match_artist TEXT,
            match_uri    TEXT
        );
    )");
    exec("CREATE INDEX IF NOT EXISTS music_service_tracks_track_id ON music_service_tracks(track_id);");
    exec("CREATE INDEX IF NOT EXISTS music_service_tracks_track_key ON music_service_tracks(track_key);");

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

    // Playlists — user-created track lists
    exec(R"(
        CREATE TABLE IF NOT EXISTS playlists (
            id         TEXT    PRIMARY KEY,
            name       TEXT    NOT NULL,
            created_at TEXT    NOT NULL DEFAULT (datetime('now'))
        );
    )");

    // Migration: the federated handle a playlist belongs to (ignored if it exists)
    sqlite3_exec(m_db, "ALTER TABLE playlists ADD COLUMN handle TEXT", nullptr, nullptr, nullptr);

    exec(R"(
        CREATE TABLE IF NOT EXISTS playlist_items (
            playlist_id TEXT    NOT NULL,
            position    INTEGER NOT NULL,
            ref         TEXT    NOT NULL,
            PRIMARY KEY (playlist_id, position),
            FOREIGN KEY (playlist_id) REFERENCES playlists(id) ON DELETE CASCADE
        );
    )");

    // Rootlist — the user's ordered list of playlists (and other URIs) in the sidebar
    exec(R"(
        CREATE TABLE IF NOT EXISTS rootlist (
            id         INTEGER PRIMARY KEY AUTOINCREMENT,
            uri        TEXT    NOT NULL UNIQUE,
            title      TEXT    NOT NULL,
            position   INTEGER NOT NULL DEFAULT 0,
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

    // Migration: track position on its release. Runs after the rebuild above,
    // which copies only the original three columns (ignored if it exists).
    sqlite3_exec(m_db, "ALTER TABLE tracks ADD COLUMN number INTEGER NOT NULL DEFAULT 0",
                 nullptr, nullptr, nullptr);
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

// ── Rootlist ──────────────────────────────────────────────────────────────────

namespace {

int rootlistCount(sqlite3* db)
{
    sqlite3_stmt* stmt = nullptr;
    int count = 0;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM rootlist;", -1, &stmt, nullptr) != SQLITE_OK) return 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return count;
}

// Position of the node for uri, or -1 if it is not listed.
int rootlistPosition(sqlite3* db, const std::string& uri)
{
    sqlite3_stmt* stmt = nullptr;
    int position = -1;
    if (sqlite3_prepare_v2(db, "SELECT position FROM rootlist WHERE uri = ?;", -1, &stmt, nullptr) != SQLITE_OK)
        return -1;
    sqlite3_bind_text(stmt, 1, uri.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW)
        position = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return position;
}

// Runs sql with up to three integer parameters and a trailing text parameter.
bool runRootlistUpdate(sqlite3* db, const char* sql, std::initializer_list<int> ints, const std::string* text = nullptr)
{
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    int index = 1;
    for (int value : ints)
        sqlite3_bind_int(stmt, index++, value);
    if (text)
        sqlite3_bind_text(stmt, index, text->c_str(), -1, SQLITE_TRANSIENT);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

} // namespace

std::vector<Database::RootlistNode> Database::getRootlist()
{
    std::vector<RootlistNode> result;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "SELECT id, uri, title, position FROM rootlist ORDER BY position, id;",
        -1, &stmt, nullptr) != SQLITE_OK) return result;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        RootlistNode node;
        node.id       = sqlite3_column_int(stmt, 0);
        node.uri      = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        node.title    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        node.position = sqlite3_column_int(stmt, 3);
        result.push_back(node);
    }
    sqlite3_finalize(stmt);
    return result;
}

bool Database::insertRootlistNode(const std::string& uri, const std::string& title, int position)
{
    if (rootlistPosition(m_db, uri) >= 0) return false;
    position = std::clamp(position, 0, rootlistCount(m_db));

    exec("BEGIN;");
    bool ok = runRootlistUpdate(m_db, "UPDATE rootlist SET position = position + 1 WHERE position >= ?;", {position});
    sqlite3_stmt* stmt = nullptr;
    if (ok && sqlite3_prepare_v2(m_db,
        "INSERT INTO rootlist (uri, title, position) VALUES (?, ?, ?);",
        -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, uri.c_str(),   -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, position);
        ok = sqlite3_step(stmt) == SQLITE_DONE;
        sqlite3_finalize(stmt);
    } else {
        ok = false;
    }
    exec(ok ? "COMMIT;" : "ROLLBACK;");
    return ok;
}

void Database::renameRootlistNode(const std::string& uri, const std::string& title)
{
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, "UPDATE rootlist SET title = ? WHERE uri = ?;", -1, &stmt, nullptr) != SQLITE_OK)
        return;
    sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, uri.c_str(),   -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void Database::moveRootlistNode(const std::string& uri, int position)
{
    const int from = rootlistPosition(m_db, uri);
    if (from < 0) return;
    const int to = std::clamp(position, 0, rootlistCount(m_db) - 1);
    if (to == from) return;

    exec("BEGIN;");
    // Shift the nodes between the old and the new position towards the gap.
    bool ok = to > from
        ? runRootlistUpdate(m_db, "UPDATE rootlist SET position = position - 1 WHERE position > ? AND position <= ?;", {from, to})
        : runRootlistUpdate(m_db, "UPDATE rootlist SET position = position + 1 WHERE position >= ? AND position < ?;", {to, from});
    ok = ok && runRootlistUpdate(m_db, "UPDATE rootlist SET position = ? WHERE uri = ?;", {to}, &uri);
    exec(ok ? "COMMIT;" : "ROLLBACK;");
}

void Database::removeRootlistNode(const std::string& uri)
{
    const int position = rootlistPosition(m_db, uri);
    if (position < 0) return;

    exec("BEGIN;");
    bool ok = runRootlistUpdate(m_db, "DELETE FROM rootlist WHERE uri = ?;", {}, &uri);
    ok = ok && runRootlistUpdate(m_db, "UPDATE rootlist SET position = position - 1 WHERE position > ?;", {position});
    exec(ok ? "COMMIT;" : "ROLLBACK;");
}

// ── Playlists ─────────────────────────────────────────────────────────────────

std::string Database::createPlaylist(const std::string& name, const std::string& handle)
{
    const std::string id = generateUUID4();
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, "INSERT INTO playlists (id, name, handle) VALUES (?, ?, ?);", -1, &stmt, nullptr) != SQLITE_OK)
        return {};
    sqlite3_bind_text(stmt, 1, id.c_str(),     -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, name.c_str(),   -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, handle.c_str(), -1, SQLITE_TRANSIENT);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok ? id : std::string();
}

void Database::setPlaylistHandle(const std::string& id, const std::string& handle)
{
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, "UPDATE playlists SET handle = ? WHERE id = ?;", -1, &stmt, nullptr) != SQLITE_OK)
        return;
    sqlite3_bind_text(stmt, 1, handle.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, id.c_str(),     -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void Database::setRootlistNodeUri(const std::string& uri, const std::string& newUri)
{
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, "UPDATE rootlist SET uri = ? WHERE uri = ?;", -1, &stmt, nullptr) != SQLITE_OK)
        return;
    sqlite3_bind_text(stmt, 1, newUri.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, uri.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void Database::renamePlaylist(const std::string& id, const std::string& name)
{
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, "UPDATE playlists SET name = ? WHERE id = ?;", -1, &stmt, nullptr) != SQLITE_OK)
        return;
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, id.c_str(),   -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void Database::deletePlaylist(const std::string& id)
{
    sqlite3_stmt* items = nullptr;
    if (sqlite3_prepare_v2(m_db, "DELETE FROM playlist_items WHERE playlist_id = ?;", -1, &items, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(items, 1, id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(items);
    }
    sqlite3_finalize(items);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, "DELETE FROM playlists WHERE id = ?;", -1, &stmt, nullptr) != SQLITE_OK)
        return;
    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::string Database::getPlaylistName(const std::string& id)
{
    sqlite3_stmt* stmt = nullptr;
    std::string name;
    if (sqlite3_prepare_v2(m_db, "SELECT name FROM playlists WHERE id = ?;", -1, &stmt, nullptr) != SQLITE_OK)
        return name;
    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW)
        name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    sqlite3_finalize(stmt);
    return name;
}

bool Database::getPlaylistRefs(const std::string& id, std::vector<std::string>& refs)
{
    refs.clear();
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "SELECT ref FROM playlist_items WHERE playlist_id = ? ORDER BY position;",
        -1, &stmt, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    int step = SQLITE_OK;
    while ((step = sqlite3_step(stmt)) == SQLITE_ROW) {
        const auto* value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        refs.emplace_back(value ? value : "");
    }
    sqlite3_finalize(stmt);
    return step == SQLITE_DONE;
}

bool Database::insertPlaylistRefs(const std::string& id, const std::vector<std::string>& refs, int position)
{
    if (refs.empty() || getPlaylistName(id).empty())
        return false;

    std::vector<std::string> ordered;
    if (!getPlaylistRefs(id, ordered))
        return false;
    const size_t index = static_cast<size_t>(std::clamp(position, 0, static_cast<int>(ordered.size())));
    ordered.insert(ordered.begin() + index, refs.begin(), refs.end());

    if (sqlite3_exec(m_db, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr) != SQLITE_OK)
        return false;

    bool ok = false;
    sqlite3_stmt* remove = nullptr;
    sqlite3_stmt* insert = nullptr;
    if (sqlite3_prepare_v2(m_db, "DELETE FROM playlist_items WHERE playlist_id = ?;", -1, &remove, nullptr) == SQLITE_OK &&
        sqlite3_prepare_v2(m_db,
            "INSERT INTO playlist_items (playlist_id, position, ref) VALUES (?, ?, ?);",
            -1, &insert, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(remove, 1, id.c_str(), -1, SQLITE_TRANSIENT);
        ok = sqlite3_step(remove) == SQLITE_DONE;
        for (size_t i = 0; ok && i < ordered.size(); ++i) {
            sqlite3_reset(insert);
            sqlite3_clear_bindings(insert);
            sqlite3_bind_text(insert, 1, id.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(insert, 2, static_cast<int>(i));
            sqlite3_bind_text(insert, 3, ordered[i].c_str(), -1, SQLITE_TRANSIENT);
            ok = sqlite3_step(insert) == SQLITE_DONE;
        }
    }
    sqlite3_finalize(remove);
    sqlite3_finalize(insert);
    if (!ok) {
        sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
    if (sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
    return true;
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

Database::ArtistRecord Database::getArtist(const std::string& isni)
{
    ArtistRecord out;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "SELECT isni, name FROM artists WHERE isni = ?;",
        -1, &stmt, nullptr) != SQLITE_OK) return out;
    sqlite3_bind_text(stmt, 1, isni.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW)
        out = { colText(stmt, 0), colText(stmt, 1) };
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

    out.recordingArtists = getRecordingArtists(out.recording.isrc);
    if (!out.release.upc.empty())
        out.releaseArtists = getReleaseArtists(out.release.upc);

    return out;
}

// ── Music browsing ────────────────────────────────────────────────────────────

// Run a statement taking one text parameter (writes only; result rows ignored).
static void execWithText(sqlite3* db, const char* sql, const std::string& v)
{
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &s, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text(s, 1, v.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(s);
    sqlite3_finalize(s);
}

// Comma-joined credited artist names per content key, in credit order.
// One query per junction table instead of one per row; GROUP_CONCAT would
// not guarantee the credit order.
static std::unordered_map<std::string, std::string>
artistNamesByKey(sqlite3* db, const std::string& junction, const std::string& keyCol)
{
    std::unordered_map<std::string, std::string> out;
    const std::string sql =
        "SELECT j." + keyCol + ", a.name FROM " + junction + " j"
        " JOIN artists a ON a.isni = j.artist_id"
        " ORDER BY j." + keyCol + ", j.number, a.name;";
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &s, nullptr) != SQLITE_OK) return out;
    while (sqlite3_step(s) == SQLITE_ROW) {
        std::string& names = out[colText(s, 0)];
        if (!names.empty()) names += ", ";
        names += colText(s, 1);
    }
    sqlite3_finalize(s);
    return out;
}

static std::vector<Database::ArtistRelationship>
creditsFor(sqlite3* db, const char* sql, const std::string& key)
{
    std::vector<Database::ArtistRelationship> out;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &s, nullptr) != SQLITE_OK) return out;
    sqlite3_bind_text(s, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(s) == SQLITE_ROW) {
        out.push_back({ colText(s, 0), colText(s, 1),
                        sqlite3_column_int(s, 2) != 0,
                        sqlite3_column_int(s, 3) });
    }
    sqlite3_finalize(s);
    return out;
}

// Shared SELECT for every track list; callers append WHERE / ORDER BY.
static const char* kTrackRowSelect = R"(
    SELECT t.id, COALESCE(t.number, 0),
           rec.isrc, rec.name, COALESCE(rec.version, ''),
           COALESCE(rel.upc, ''), COALESCE(rel.name, ''),
           COALESCE((SELECT p.uri FROM playable_tracks pt
                     JOIN playables p ON p.id = pt.playable_id
                     WHERE pt.track_id = t.id LIMIT 1), ''),
           EXISTS (SELECT 1 FROM saves s
                   WHERE s.track_id = t.id AND s.deleted_at IS NULL)
    FROM tracks t
    JOIN recordings    rec ON rec.isrc = t.recording_id
    LEFT JOIN releases rel ON rel.upc  = t.release_id
)";

// @p key is bound to every ?1 in @p tail when non-empty.
static std::vector<Database::TrackRow>
queryTrackRows(sqlite3* db, const std::string& tail, const std::string& key = "")
{
    std::vector<Database::TrackRow> out;
    const std::string sql = std::string(kTrackRowSelect) + tail;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &s, nullptr) != SQLITE_OK) return out;
    if (!key.empty())
        sqlite3_bind_text(s, 1, key.c_str(), -1, SQLITE_TRANSIENT);

    auto names = artistNamesByKey(db, "recording_artists", "recording_id");
    while (sqlite3_step(s) == SQLITE_ROW) {
        Database::TrackRow r;
        r.id          = sqlite3_column_int(s, 0);
        r.number      = sqlite3_column_int(s, 1);
        r.isrc        = colText(s, 2);
        r.name        = colText(s, 3);
        r.version     = colText(s, 4);
        r.upc         = colText(s, 5);
        r.releaseName = colText(s, 6);
        r.playUri     = colText(s, 7);
        r.saved       = sqlite3_column_int(s, 8) != 0;
        auto it = names.find(r.isrc);
        if (it != names.end()) r.artists = it->second;
        out.push_back(std::move(r));
    }
    sqlite3_finalize(s);
    return out;
}

static std::vector<Database::ReleaseSummary>
queryReleaseSummaries(sqlite3* db, const std::string& tail, const std::string& key = "")
{
    std::vector<Database::ReleaseSummary> out;
    const std::string sql =
        "SELECT rel.upc, rel.name, COALESCE(rel.release_date, ''),"
        "       (SELECT COUNT(*) FROM tracks t WHERE t.release_id = rel.upc)"
        " FROM releases rel " + tail;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &s, nullptr) != SQLITE_OK) return out;
    if (!key.empty())
        sqlite3_bind_text(s, 1, key.c_str(), -1, SQLITE_TRANSIENT);

    auto names = artistNamesByKey(db, "release_artists", "release_id");
    while (sqlite3_step(s) == SQLITE_ROW) {
        Database::ReleaseSummary r;
        r.release    = { colText(s, 0), colText(s, 1), colText(s, 2) };
        r.trackCount = sqlite3_column_int(s, 3);
        auto it = names.find(r.release.upc);
        if (it != names.end()) r.artists = it->second;
        out.push_back(std::move(r));
    }
    sqlite3_finalize(s);
    return out;
}

std::string Database::generateLocalId()
{
    return "local:" + generateUUID4();
}

Database::RecordingRecord Database::getRecording(const std::string& isrc)
{
    RecordingRecord out;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "SELECT isrc, name, COALESCE(version,'') FROM recordings WHERE isrc = ?;",
        -1, &s, nullptr) != SQLITE_OK) return out;
    sqlite3_bind_text(s, 1, isrc.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(s) == SQLITE_ROW)
        out = { colText(s, 0), colText(s, 1), colText(s, 2) };
    sqlite3_finalize(s);
    return out;
}

Database::ReleaseRecord Database::getRelease(const std::string& upc)
{
    ReleaseRecord out;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "SELECT upc, name, COALESCE(release_date,'') FROM releases WHERE upc = ?;",
        -1, &s, nullptr) != SQLITE_OK) return out;
    sqlite3_bind_text(s, 1, upc.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(s) == SQLITE_ROW)
        out = { colText(s, 0), colText(s, 1), colText(s, 2) };
    sqlite3_finalize(s);
    return out;
}

std::vector<Database::ArtistSummary> Database::getArtistSummaries()
{
    std::vector<ArtistSummary> out;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(m_db, R"(
        SELECT a.isni, a.name,
               (SELECT COUNT(*) FROM recording_artists ra WHERE ra.artist_id = a.isni),
               (SELECT COUNT(*) FROM release_artists   rl WHERE rl.artist_id = a.isni)
        FROM artists a
        ORDER BY a.name COLLATE NOCASE;
    )", -1, &s, nullptr) != SQLITE_OK) return out;
    while (sqlite3_step(s) == SQLITE_ROW) {
        ArtistSummary a;
        a.artist         = { colText(s, 0), colText(s, 1) };
        a.recordingCount = sqlite3_column_int(s, 2);
        a.releaseCount   = sqlite3_column_int(s, 3);
        out.push_back(std::move(a));
    }
    sqlite3_finalize(s);
    return out;
}

std::vector<Database::RecordingSummary> Database::getRecordingSummaries()
{
    std::vector<RecordingSummary> out;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "SELECT isrc, name, COALESCE(version,'') FROM recordings ORDER BY name COLLATE NOCASE;",
        -1, &s, nullptr) != SQLITE_OK) return out;
    auto names = artistNamesByKey(m_db, "recording_artists", "recording_id");
    while (sqlite3_step(s) == SQLITE_ROW) {
        RecordingSummary r;
        r.recording = { colText(s, 0), colText(s, 1), colText(s, 2) };
        auto it = names.find(r.recording.isrc);
        if (it != names.end()) r.artists = it->second;
        out.push_back(std::move(r));
    }
    sqlite3_finalize(s);
    return out;
}

std::vector<Database::ReleaseSummary> Database::getReleaseSummaries()
{
    return queryReleaseSummaries(m_db, "ORDER BY rel.name COLLATE NOCASE;");
}

std::vector<Database::ArtistRelationship> Database::getRecordingArtists(const std::string& isrc)
{
    return creditsFor(m_db, R"(
        SELECT a.isni, a.name, ra.is_primary, ra.number
        FROM recording_artists ra
        JOIN artists a ON a.isni = ra.artist_id
        WHERE ra.recording_id = ?
        ORDER BY ra.number;
    )", isrc);
}

std::vector<Database::ArtistRelationship> Database::getReleaseArtists(const std::string& upc)
{
    return creditsFor(m_db, R"(
        SELECT a.isni, a.name, ra.is_primary, ra.number
        FROM release_artists ra
        JOIN artists a ON a.isni = ra.artist_id
        WHERE ra.release_id = ?
        ORDER BY ra.number;
    )", upc);
}

std::vector<Database::TrackRow> Database::getAllTracks()
{
    return queryTrackRows(m_db,
        "ORDER BY rel.name IS NULL, rel.name COLLATE NOCASE,"
        " CASE WHEN t.number > 0 THEN 0 ELSE 1 END, t.number, t.id;");
}

std::vector<Database::TrackRow> Database::getTracksForRelease(const std::string& upc)
{
    // Numbered tracks in order, unnumbered ones after them in insertion order.
    return queryTrackRows(m_db,
        "WHERE t.release_id = ?1"
        " ORDER BY CASE WHEN t.number > 0 THEN 0 ELSE 1 END, t.number, t.id;", upc);
}

std::vector<Database::TrackRow> Database::getTracksForArtist(const std::string& isni)
{
    return queryTrackRows(m_db,
        "WHERE rec.isrc IN (SELECT recording_id FROM recording_artists WHERE artist_id = ?1)"
        " ORDER BY rec.name COLLATE NOCASE, rel.name COLLATE NOCASE, t.id;", isni);
}

std::vector<Database::TrackRow> Database::getTracksForRecording(const std::string& isrc)
{
    return queryTrackRows(m_db,
        "WHERE rec.isrc = ?1 ORDER BY rel.release_date, rel.name COLLATE NOCASE, t.id;", isrc);
}

std::vector<Database::TrackRow> Database::getTracksByTitle(const std::string& title)
{
    return queryTrackRows(m_db,
        "WHERE rec.name = ?1 COLLATE NOCASE ORDER BY rel.release_date, rel.name COLLATE NOCASE, t.id;",
        title);
}

std::vector<Database::ReleaseSummary> Database::getReleasesForArtist(const std::string& isni)
{
    return queryReleaseSummaries(m_db,
        "WHERE rel.upc IN ("
        "   SELECT release_id FROM release_artists WHERE artist_id = ?1"
        "   UNION"
        "   SELECT t.release_id FROM tracks t"
        "   JOIN recording_artists ra ON ra.recording_id = t.recording_id"
        "   WHERE ra.artist_id = ?1 AND t.release_id IS NOT NULL)"
        " ORDER BY rel.release_date, rel.name COLLATE NOCASE;", isni);
}

void Database::setTrackNumber(int64_t trackId, int number)
{
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(m_db, "UPDATE tracks SET number = ? WHERE id = ?;",
                           -1, &s, nullptr) != SQLITE_OK) return;
    sqlite3_bind_int(s, 1, number);
    sqlite3_bind_int64(s, 2, trackId);
    sqlite3_step(s);
    sqlite3_finalize(s);
}

void Database::setTrackRelease(int64_t trackId, const std::string& releaseUpc)
{
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(m_db, "UPDATE tracks SET release_id = ? WHERE id = ?;",
                           -1, &s, nullptr) != SQLITE_OK) return;
    bindOptText(s, 1, releaseUpc);
    sqlite3_bind_int64(s, 2, trackId);
    sqlite3_step(s);
    sqlite3_finalize(s);
}

void Database::deleteArtist(const std::string& isni)
{
    execWithText(m_db, "DELETE FROM artists WHERE isni = ?;", isni);
}

void Database::deleteRecording(const std::string& isrc)
{
    execWithText(m_db, "DELETE FROM recordings WHERE isrc = ?;", isrc);
}

void Database::deleteRelease(const std::string& upc)
{
    execWithText(m_db, "DELETE FROM releases WHERE upc = ?;", upc);
}

void Database::deleteTrack(int64_t trackId)
{
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(m_db, "DELETE FROM tracks WHERE id = ?;",
                           -1, &s, nullptr) != SQLITE_OK) return;
    sqlite3_bind_int64(s, 1, trackId);
    sqlite3_step(s);
    sqlite3_finalize(s);
}

void Database::removeSave(int64_t trackId)
{
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "UPDATE saves SET deleted_at = datetime('now')"
        " WHERE track_id = ? AND deleted_at IS NULL;",
        -1, &s, nullptr) != SQLITE_OK) return;
    sqlite3_bind_int64(s, 1, trackId);
    sqlite3_step(s);
    sqlite3_finalize(s);
}

// ── Custom user-defined table support ─────────────────────────────────────────

// Column definitions are stored in db_tables.columns as a simple
// pipe-delimited string:  "name1:TYPE1|name2:TYPE2|..."
// (No JSON library dependency; types are restricted to TEXT/INTEGER/REAL.)

std::string Database::encodeColumns(const std::vector<std::pair<std::string,std::string>>& cols)
{
    std::string out;
    for (size_t i = 0; i < cols.size(); ++i) {
        if (i) out += '|';
        out += cols[i].first + ':' + cols[i].second;
    }
    return out;
}

std::vector<std::pair<std::string,std::string>>
Database::decodeColumns(const std::string& encoded)
{
    std::vector<std::pair<std::string,std::string>> cols;
    if (encoded.empty()) return cols;
    size_t pos = 0;
    while (pos <= encoded.size()) {
        size_t pipe  = encoded.find('|', pos);
        std::string token = (pipe == std::string::npos)
            ? encoded.substr(pos)
            : encoded.substr(pos, pipe - pos);
        size_t colon = token.find(':');
        if (colon != std::string::npos) {
            cols.emplace_back(token.substr(0, colon), token.substr(colon + 1));
        }
        if (pipe == std::string::npos) break;
        pos = pipe + 1;
    }
    return cols;
}

void Database::initializeCustomTables()
{
    // ── 1. Ensure the meta-table exists ──────────────────────────────────────
    exec(R"(
        CREATE TABLE IF NOT EXISTS db_tables (
            id      INTEGER PRIMARY KEY AUTOINCREMENT,
            name    TEXT    NOT NULL UNIQUE,
            columns TEXT    NOT NULL DEFAULT ''
        );
    )");

    // ── 2. Add extended metadata columns if they are missing (idempotent) ────
    {
        // Helper: collect existing column names in db_tables
        std::vector<std::string> existing;
        {
            sqlite3_stmt* ps = nullptr;
            if (sqlite3_prepare_v2(m_db, "PRAGMA table_info(db_tables);",
                                   -1, &ps, nullptr) == SQLITE_OK) {
                while (sqlite3_step(ps) == SQLITE_ROW) {
                    const unsigned char* n = sqlite3_column_text(ps, 1);
                    if (n) existing.emplace_back(reinterpret_cast<const char*>(n));
                }
                sqlite3_finalize(ps);
            }
        }
        auto hasCol = [&](const char* col) {
            for (auto& e : existing) if (e == col) return true;
            return false;
        };
        auto addCol = [&](const char* ddl) {
            std::string sql = std::string("ALTER TABLE db_tables ADD COLUMN ") + ddl + ";";
            sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, nullptr);
        };
        if (!hasCol("display_name"))  addCol("display_name  TEXT NOT NULL DEFAULT ''");
        if (!hasCol("noun_singular"))  addCol("noun_singular TEXT NOT NULL DEFAULT ''");
        if (!hasCol("noun_plural"))    addCol("noun_plural   TEXT NOT NULL DEFAULT ''");
        if (!hasCol("description"))    addCol("description   TEXT NOT NULL DEFAULT ''");
    }

    // ── 3. Migrate any user_ tables still using the old INTEGER PK ───────────
    std::vector<std::pair<std::string,std::string>> tables; // {name, encoded_columns}
    {
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(m_db,
            "SELECT name, columns FROM db_tables;",
            -1, &s, nullptr) == SQLITE_OK) {
            while (sqlite3_step(s) == SQLITE_ROW) {
                const unsigned char* n = sqlite3_column_text(s, 0);
                const unsigned char* c = sqlite3_column_text(s, 1);
                std::string name = n ? reinterpret_cast<const char*>(n) : "";
                std::string cols = c ? reinterpret_cast<const char*>(c) : "";
                if (!name.empty())
                    tables.emplace_back(std::move(name), std::move(cols));
            }
            sqlite3_finalize(s);
        }
    }

    for (auto& [name, encoded] : tables) {
        bool needsMigration = false;
        {
            std::string pragma = "PRAGMA table_info(user_" + name + ");";
            sqlite3_stmt* ps = nullptr;
            if (sqlite3_prepare_v2(m_db, pragma.c_str(), -1, &ps, nullptr) == SQLITE_OK) {
                while (sqlite3_step(ps) == SQLITE_ROW) {
                    const unsigned char* colName = sqlite3_column_text(ps, 1);
                    const unsigned char* colType = sqlite3_column_text(ps, 2);
                    std::string cn = colName ? reinterpret_cast<const char*>(colName) : "";
                    std::string ct = colType ? reinterpret_cast<const char*>(colType) : "";
                    if (cn == "id" && ct == "INTEGER") needsMigration = true;
                }
                sqlite3_finalize(ps);
            }
        }
        if (!needsMigration) continue;

        std::string drop = "DROP TABLE IF EXISTS user_" + name + ";";
        sqlite3_exec(m_db, drop.c_str(), nullptr, nullptr, nullptr);

        auto cols = decodeColumns(encoded);
        std::string ddl = "CREATE TABLE IF NOT EXISTS user_" + name + " (id TEXT PRIMARY KEY";
        for (auto& col : cols) {
            std::string safeType = "TEXT"; // BELONGS_TO → TEXT in SQLite
            if (col.second == "INTEGER") safeType = "INTEGER";
            else if (col.second == "REAL") safeType = "REAL";
            ddl += ", " + col.first + " " + safeType;
        }
        ddl += ");";
        exec(ddl.c_str());
    }
}

std::vector<CustomTableDef> Database::getCustomTables()
{
    std::vector<CustomTableDef> out;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "SELECT id, name, columns, display_name, noun_singular, noun_plural, description"
        " FROM db_tables ORDER BY name;",
        -1, &stmt, nullptr) != SQLITE_OK) return out;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        CustomTableDef d;
        d.id      = sqlite3_column_int(stmt, 0);
        auto txt  = [&](int col) -> std::string {
            const unsigned char* t = sqlite3_column_text(stmt, col);
            return t ? reinterpret_cast<const char*>(t) : "";
        };
        d.name         = txt(1);
        d.columns      = decodeColumns(txt(2));
        d.displayName  = txt(3);
        d.nounSingular = txt(4);
        d.nounPlural   = txt(5);
        d.description  = txt(6);
        out.push_back(std::move(d));
    }
    sqlite3_finalize(stmt);
    return out;
}

CustomTableDef Database::getCustomTable(const std::string& name)
{
    CustomTableDef d;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "SELECT id, name, columns, display_name, noun_singular, noun_plural, description"
        " FROM db_tables WHERE name = ? LIMIT 1;",
        -1, &stmt, nullptr) != SQLITE_OK) return d;
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        auto txt = [&](int col) -> std::string {
            const unsigned char* t = sqlite3_column_text(stmt, col);
            return t ? reinterpret_cast<const char*>(t) : "";
        };
        d.id           = sqlite3_column_int(stmt, 0);
        d.name         = txt(1);
        d.columns      = decodeColumns(txt(2));
        d.displayName  = txt(3);
        d.nounSingular = txt(4);
        d.nounPlural   = txt(5);
        d.description  = txt(6);
    }
    sqlite3_finalize(stmt);
    return d;
}

int Database::createCustomTable(const std::string& name,
                                 const std::vector<std::pair<std::string,std::string>>& cols,
                                 const std::string& displayName,
                                 const std::string& nounSingular,
                                 const std::string& nounPlural,
                                 const std::string& description)
{
    if (name.empty() || cols.empty()) return -1;

    // Store meta entry including display metadata
    std::string encoded = encodeColumns(cols);
    {
        sqlite3_stmt* ins = nullptr;
        if (sqlite3_prepare_v2(m_db,
            "INSERT INTO db_tables"
            " (name, columns, display_name, noun_singular, noun_plural, description)"
            " VALUES (?, ?, ?, ?, ?, ?);",
            -1, &ins, nullptr) != SQLITE_OK) return -1;
        sqlite3_bind_text(ins, 1, name.c_str(),         -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(ins, 2, encoded.c_str(),      -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(ins, 3, displayName.c_str(),  -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(ins, 4, nounSingular.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(ins, 5, nounPlural.c_str(),   -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(ins, 6, description.c_str(),  -1, SQLITE_TRANSIENT);
        int rc = sqlite3_step(ins);
        sqlite3_finalize(ins);
        if (rc != SQLITE_DONE) return -1;
    }

    // Create the physical table with prefix "user_" — UUID4 TEXT primary key
    // BELONGS_TO columns are stored as plain TEXT in SQLite.
    std::string ddl = "CREATE TABLE IF NOT EXISTS user_" + name + " (id TEXT PRIMARY KEY";
    for (auto& col : cols) {
        std::string safeType = "TEXT";
        if (col.second == "INTEGER") safeType = "INTEGER";
        else if (col.second == "REAL") safeType = "REAL";
        // BELONGS_TO:<target> → TEXT
        ddl += ", " + col.first + " " + safeType;
    }
    ddl += ");";
    exec(ddl.c_str());

    return static_cast<int>(sqlite3_last_insert_rowid(m_db));
}

void Database::updateCustomTableMeta(const CustomTableDef& def)
{
    if (def.name.empty()) return;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "UPDATE db_tables SET display_name=?, noun_singular=?, noun_plural=?, description=?"
        " WHERE name=?;",
        -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text(stmt, 1, def.displayName.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, def.nounSingular.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, def.nounPlural.c_str(),   -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, def.description.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, def.name.c_str(),         -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void Database::deleteCustomTable(const std::string& name)
{
    if (name.empty()) return;

    // Drop physical table
    std::string ddl = "DROP TABLE IF EXISTS user_" + name + ";";
    sqlite3_exec(m_db, ddl.c_str(), nullptr, nullptr, nullptr);

    // Remove meta entry
    sqlite3_stmt* del = nullptr;
    if (sqlite3_prepare_v2(m_db,
        "DELETE FROM db_tables WHERE name = ?;",
        -1, &del, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(del, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(del);
        sqlite3_finalize(del);
    }
}

std::vector<CustomRow> Database::getCustomRows(const std::string& tableName)
{
    std::vector<CustomRow> out;
    if (tableName.empty()) return out;

    std::string sql = "SELECT * FROM user_" + tableName + " ORDER BY id;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        return out;

    int colCount = sqlite3_column_count(stmt);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        CustomRow r;
        const unsigned char* idTxt = sqlite3_column_text(stmt, 0);
        r.id = idTxt ? reinterpret_cast<const char*>(idTxt) : "";
        // columns 1..N are the user columns
        for (int i = 1; i < colCount; ++i) {
            const unsigned char* t = sqlite3_column_text(stmt, i);
            r.values.push_back(t ? reinterpret_cast<const char*>(t) : "");
        }
        out.push_back(std::move(r));
    }
    sqlite3_finalize(stmt);
    return out;
}

CustomRow Database::getCustomRow(const std::string& tableName, const std::string& rowUuid)
{
    CustomRow r;
    if (tableName.empty() || rowUuid.empty()) return r;

    std::string sql = "SELECT * FROM user_" + tableName + " WHERE id=? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        return r;
    sqlite3_bind_text(stmt, 1, rowUuid.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        r.id = rowUuid;
        int colCount = sqlite3_column_count(stmt);
        for (int i = 1; i < colCount; ++i) {
            const unsigned char* t = sqlite3_column_text(stmt, i);
            r.values.push_back(t ? reinterpret_cast<const char*>(t) : "");
        }
    }
    sqlite3_finalize(stmt);
    return r;
}

std::string Database::insertCustomRow(const std::string& tableName,
                               const CustomTableDef& def,
                               const std::vector<std::string>& values)
{
    if (tableName.empty() || def.columns.empty()) return "";

    std::string uuid = generateUUID4();

    // Build "INSERT INTO user_<name> (id, col1, col2, ...) VALUES (?, ?, ?,...)"
    std::string cols = "id", placeholders = "?";
    for (size_t i = 0; i < def.columns.size(); ++i) {
        cols         += ',' + def.columns[i].first;
        placeholders += ",?";
    }
    std::string sql = "INSERT INTO user_" + tableName +
                      " (" + cols + ") VALUES (" + placeholders + ");";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return "";
    // Bind UUID as first param
    sqlite3_bind_text(stmt, 1, uuid.c_str(), -1, SQLITE_TRANSIENT);
    // Bind user column values starting at param 2
    for (size_t i = 0; i < values.size(); ++i) {
        sqlite3_bind_text(stmt, static_cast<int>(i + 2),
                          values[i].c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return uuid;
}

void Database::updateCustomRow(const std::string& tableName,
                                const CustomTableDef& def,
                                const std::string& rowUuid,
                                const std::vector<std::string>& values)
{
    if (tableName.empty() || def.columns.empty() || rowUuid.empty()) return;

    std::string setClause;
    for (size_t i = 0; i < def.columns.size(); ++i) {
        if (i) setClause += ',';
        setClause += def.columns[i].first + "=?";
    }
    std::string sql = "UPDATE user_" + tableName + " SET " + setClause + " WHERE id=?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return;
    for (size_t i = 0; i < values.size(); ++i) {
        sqlite3_bind_text(stmt, static_cast<int>(i + 1),
                          values[i].c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_text(stmt, static_cast<int>(values.size() + 1),
                      rowUuid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void Database::deleteCustomRow(const std::string& tableName, const std::string& rowUuid)
{
    if (tableName.empty() || rowUuid.empty()) return;
    std::string sql = "DELETE FROM user_" + tableName + " WHERE id=?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text(stmt, 1, rowUuid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}
