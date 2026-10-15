#include "SPAudioFile.h"
#include "SPDBConnection.h"
#include <utility>

// ── Constructors ──────────────────────────────────────────────────────────────

SPAudioFile::SPAudioFile(int64_t fileId, std::string name, std::string version,
                         std::string artists, std::string album,
                         int64_t duration, std::string coverArt)
    : fileId(fileId), name(std::move(name)), version(std::move(version)),
      artists(std::move(artists)), album(std::move(album)),
      duration(duration), coverArt(std::move(coverArt)) {}

// ── Persistence ───────────────────────────────────────────────────────────────

bool SPAudioFile::save(SPDBConnection& conn) {
    auto bindText = [](sqlite3_stmt* s, int i, const std::string& v) {
        if (!v.empty())
            sqlite3_bind_text(s, i, v.c_str(), -1, SQLITE_TRANSIENT);
        else
            sqlite3_bind_null(s, i);
    };

    if (!isPersisted()) {
        sqlite3_stmt* stmt = conn.prepare(
            "INSERT INTO audio_files "
            "(file_id, name, version, artists, album, duration, cover_art) "
            "VALUES (?, ?, ?, ?, ?, ?, ?)");
        sqlite3_bind_int64(stmt, 1, fileId);
        bindText(stmt, 2, name);
        bindText(stmt, 3, version);
        bindText(stmt, 4, artists);
        bindText(stmt, 5, album);
        sqlite3_bind_int64(stmt, 6, duration);
        bindText(stmt, 7, coverArt);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        if (rc == SQLITE_DONE)
            id = sqlite3_last_insert_rowid(conn.handle());
        return rc == SQLITE_DONE;
    } else {
        sqlite3_stmt* stmt = conn.prepare(
            "UPDATE audio_files SET file_id=?, name=?, version=?, artists=?, "
            "album=?, duration=?, cover_art=? WHERE id=?");
        sqlite3_bind_int64(stmt, 1, fileId);
        bindText(stmt, 2, name);
        bindText(stmt, 3, version);
        bindText(stmt, 4, artists);
        bindText(stmt, 5, album);
        sqlite3_bind_int64(stmt, 6, duration);
        bindText(stmt, 7, coverArt);
        sqlite3_bind_int64(stmt, 8, id);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return rc == SQLITE_DONE;
    }
}

bool SPAudioFile::remove(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare("DELETE FROM audio_files WHERE id = ?");
    sqlite3_bind_int64(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

// ── Deserialise ───────────────────────────────────────────────────────────────

SPAudioFile SPAudioFile::fromStatement(sqlite3_stmt* stmt) {
    auto text = [&](int col) -> std::string {
        const char* t = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
        return t ? t : "";
    };
    SPAudioFile af;
    af.id       = sqlite3_column_int64(stmt, 0);
    af.fileId   = sqlite3_column_int64(stmt, 1);
    af.name     = text(2);
    af.version  = text(3);
    af.artists  = text(4);
    af.album    = text(5);
    af.duration = sqlite3_column_int64(stmt, 6);
    af.coverArt = text(7);
    return af;
}

// ── Queries ───────────────────────────────────────────────────────────────────

std::optional<SPAudioFile> SPAudioFile::find(SPDBConnection& conn, int64_t id) {
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT id, file_id, name, version, artists, album, duration, cover_art "
        "FROM audio_files WHERE id = ?");
    sqlite3_bind_int64(stmt, 1, id);
    std::optional<SPAudioFile> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = fromStatement(stmt);
    sqlite3_finalize(stmt);
    return result;
}

std::vector<SPAudioFile> SPAudioFile::findAll(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT id, file_id, name, version, artists, album, duration, cover_art "
        "FROM audio_files ORDER BY name");
    std::vector<SPAudioFile> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}

std::vector<SPAudioFile> SPAudioFile::findByFile(SPDBConnection& conn, int64_t fileId) {
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT id, file_id, name, version, artists, album, duration, cover_art "
        "FROM audio_files WHERE file_id = ?");
    sqlite3_bind_int64(stmt, 1, fileId);
    std::vector<SPAudioFile> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}
