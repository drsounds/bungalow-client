#include "SPPlayable.h"
#include "SPDBConnection.h"
#include "../services/SPMediaService.h"
#include <utility>

// ── Constructors ──────────────────────────────────────────────────────────────

SPPlayable::SPPlayable(std::string uri) : uri(std::move(uri)) {}

SPPlayable::SPPlayable(int64_t id, std::string uri)
    : id(id), uri(std::move(uri)) {}

// ── Service resolution ────────────────────────────────────────────────────────

void SPPlayable::resolveService(const std::vector<SPMediaService*>& services) {
    service = nullptr;
    for (SPMediaService* s : services) {
        if (s && s->acceptsUri(uri)) {
            service = s;
            return;
        }
    }
}

// ── Persistence ───────────────────────────────────────────────────────────────

bool SPPlayable::save(SPDBConnection& conn) {
    if (!isPersisted()) {
        sqlite3_stmt* stmt = conn.prepare(
            "INSERT OR IGNORE INTO playables (uri) VALUES (?)");
        sqlite3_bind_text(stmt, 1, uri.c_str(), -1, SQLITE_TRANSIENT);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        if (rc == SQLITE_DONE) {
            int64_t last = sqlite3_last_insert_rowid(conn.handle());
            if (last != 0) {
                id = last;
            } else {
                if (auto existing = findByUri(conn, uri))
                    id = existing->id;
            }
        }
        return rc == SQLITE_DONE;
    } else {
        sqlite3_stmt* stmt = conn.prepare(
            "UPDATE playables SET uri = ? WHERE id = ?");
        sqlite3_bind_text(stmt, 1, uri.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 2, id);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return rc == SQLITE_DONE;
    }
}

bool SPPlayable::remove(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare("DELETE FROM playables WHERE id = ?");
    sqlite3_bind_int64(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

// ── Deserialise ───────────────────────────────────────────────────────────────

SPPlayable SPPlayable::fromStatement(sqlite3_stmt* stmt) {
    int64_t     rowId = sqlite3_column_int64(stmt, 0);
    const char* t     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    return SPPlayable(rowId, t ? t : "");
}

// ── Queries ───────────────────────────────────────────────────────────────────

std::optional<SPPlayable> SPPlayable::find(SPDBConnection& conn, int64_t id) {
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT id, uri FROM playables WHERE id = ?");
    sqlite3_bind_int64(stmt, 1, id);
    std::optional<SPPlayable> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = fromStatement(stmt);
    sqlite3_finalize(stmt);
    return result;
}

std::optional<SPPlayable> SPPlayable::findByUri(SPDBConnection& conn,
                                                 const std::string& uri)
{
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT id, uri FROM playables WHERE uri = ?");
    sqlite3_bind_text(stmt, 1, uri.c_str(), -1, SQLITE_TRANSIENT);
    std::optional<SPPlayable> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = fromStatement(stmt);
    sqlite3_finalize(stmt);
    return result;
}

std::vector<SPPlayable> SPPlayable::findAll(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT id, uri FROM playables ORDER BY id");
    std::vector<SPPlayable> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}
