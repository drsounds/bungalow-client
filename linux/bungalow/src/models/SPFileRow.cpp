#include "SPFileRow.h"
#include "SPDBConnection.h"
#include <utility>

// ── Constructors ──────────────────────────────────────────────────────────────

SPFileRow::SPFileRow(std::string path)
    : path(std::move(path)) {}

SPFileRow::SPFileRow(int64_t id, std::string path)
    : id(id), path(std::move(path)) {}

// ── Persistence ───────────────────────────────────────────────────────────────

bool SPFileRow::save(SPDBConnection& conn) {
    if (!isPersisted()) {
        sqlite3_stmt* stmt = conn.prepare(
            "INSERT OR IGNORE INTO files (path) VALUES (?)");
        sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_TRANSIENT);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        if (rc == SQLITE_DONE) {
            // If a row already existed (IGNORE), fetch its id
            int64_t last = sqlite3_last_insert_rowid(conn.handle());
            if (last != 0) {
                id = last;
            } else {
                if (auto existing = findByPath(conn, path))
                    id = existing->id;
            }
        }
        return rc == SQLITE_DONE;
    } else {
        sqlite3_stmt* stmt = conn.prepare("UPDATE files SET path = ? WHERE id = ?");
        sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 2, id);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return rc == SQLITE_DONE;
    }
}

bool SPFileRow::remove(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare("DELETE FROM files WHERE id = ?");
    sqlite3_bind_int64(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

// ── Deserialise ───────────────────────────────────────────────────────────────

SPFileRow SPFileRow::fromStatement(sqlite3_stmt* stmt) {
    int64_t     rowId = sqlite3_column_int64(stmt, 0);
    const char* t     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    return SPFileRow(rowId, t ? t : "");
}

// ── Queries ───────────────────────────────────────────────────────────────────

std::optional<SPFileRow> SPFileRow::find(SPDBConnection& conn, int64_t id) {
    sqlite3_stmt* stmt = conn.prepare("SELECT id, path FROM files WHERE id = ?");
    sqlite3_bind_int64(stmt, 1, id);
    std::optional<SPFileRow> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = fromStatement(stmt);
    sqlite3_finalize(stmt);
    return result;
}

std::optional<SPFileRow> SPFileRow::findByPath(SPDBConnection& conn,
                                                const std::string& path)
{
    sqlite3_stmt* stmt = conn.prepare("SELECT id, path FROM files WHERE path = ?");
    sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_TRANSIENT);
    std::optional<SPFileRow> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = fromStatement(stmt);
    sqlite3_finalize(stmt);
    return result;
}

std::vector<SPFileRow> SPFileRow::findAll(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare("SELECT id, path FROM files ORDER BY path");
    std::vector<SPFileRow> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}
