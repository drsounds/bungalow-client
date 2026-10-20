#include "SPSave.h"
#include "SPDBConnection.h"

// ── Constructors ──────────────────────────────────────────────────────────────

SPSave::SPSave(int64_t trackId) : trackId(trackId) {}

// ── Persistence ───────────────────────────────────────────────────────────────

bool SPSave::save(SPDBConnection& conn) {
    if (!isPersisted()) {
        sqlite3_stmt* stmt = conn.prepare(
            "INSERT INTO saves (track_id, saved_at, deleted_at) "
            "VALUES (?, COALESCE(NULLIF(?, ''), datetime('now')), NULLIF(?, ''))");
        sqlite3_bind_int64(stmt, 1, trackId);
        sqlite3_bind_text(stmt, 2, savedAt.c_str(),   -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, deletedAt.c_str(), -1, SQLITE_TRANSIENT);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        if (rc == SQLITE_DONE)
            id = sqlite3_last_insert_rowid(conn.handle());
        return rc == SQLITE_DONE;
    } else {
        sqlite3_stmt* stmt = conn.prepare(
            "UPDATE saves SET track_id=?, deleted_at=NULLIF(?,'') WHERE id=?");
        sqlite3_bind_int64(stmt, 1, trackId);
        sqlite3_bind_text(stmt, 2, deletedAt.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 3, id);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return rc == SQLITE_DONE;
    }
}

bool SPSave::remove(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare("DELETE FROM saves WHERE id = ?");
    sqlite3_bind_int64(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

// ── Deserialise ───────────────────────────────────────────────────────────────

SPSave SPSave::fromStatement(sqlite3_stmt* stmt) {
    auto text = [&](int col) -> std::string {
        const char* t = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
        return t ? t : "";
    };
    SPSave s;
    s.id        = sqlite3_column_int64(stmt, 0);
    s.trackId   = sqlite3_column_int64(stmt, 1);
    s.savedAt   = text(2);
    s.deletedAt = text(3);
    return s;
}

// ── Queries ───────────────────────────────────────────────────────────────────

std::optional<SPSave> SPSave::find(SPDBConnection& conn, int64_t id) {
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT id, track_id, saved_at, deleted_at FROM saves WHERE id = ?");
    sqlite3_bind_int64(stmt, 1, id);
    std::optional<SPSave> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = fromStatement(stmt);
    sqlite3_finalize(stmt);
    return result;
}

std::vector<SPSave> SPSave::findAll(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT id, track_id, saved_at, deleted_at FROM saves ORDER BY saved_at DESC");
    std::vector<SPSave> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}

std::vector<SPSave> SPSave::findActive(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT id, track_id, saved_at, deleted_at FROM saves "
        "WHERE deleted_at IS NULL ORDER BY saved_at DESC");
    std::vector<SPSave> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}
