#include "models/SPLabel.h"
#include "models/SPDBConnection.h"
#include <utility>

// ── Constructors ──────────────────────────────────────────────────────────────

SPLabel::SPLabel(std::string name)
    : name(std::move(name)) {}

SPLabel::SPLabel(int64_t id, std::string name)
    : id(id), name(std::move(name)) {}

// ── Persistence ───────────────────────────────────────────────────────────────

bool SPLabel::save(SPDBConnection& conn) {
    if (!isPersisted()) {
        sqlite3_stmt* stmt = conn.prepare("INSERT INTO labels (name) VALUES (?)");
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        if (rc == SQLITE_DONE)
            id = sqlite3_last_insert_rowid(conn.handle());
        return rc == SQLITE_DONE;
    } else {
        sqlite3_stmt* stmt = conn.prepare("UPDATE labels SET name = ? WHERE id = ?");
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 2, id);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return rc == SQLITE_DONE;
    }
}

bool SPLabel::remove(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare("DELETE FROM labels WHERE id = ?");
    sqlite3_bind_int64(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

// ── Deserialise ───────────────────────────────────────────────────────────────

SPLabel SPLabel::fromStatement(sqlite3_stmt* stmt) {
    int64_t     rowId = sqlite3_column_int64(stmt, 0);
    const char* t     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    return SPLabel(rowId, t ? t : "");
}

// ── Queries ───────────────────────────────────────────────────────────────────

std::optional<SPLabel> SPLabel::find(SPDBConnection& conn, int64_t id) {
    sqlite3_stmt* stmt = conn.prepare("SELECT id, name FROM labels WHERE id = ?");
    sqlite3_bind_int64(stmt, 1, id);
    std::optional<SPLabel> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = fromStatement(stmt);
    sqlite3_finalize(stmt);
    return result;
}

std::vector<SPLabel> SPLabel::findAll(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare("SELECT id, name FROM labels ORDER BY name");
    std::vector<SPLabel> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}
