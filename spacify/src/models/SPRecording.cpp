#include "models/SPRecording.h"
#include "models/SPDBConnection.h"
#include <utility>

// ── Constructors ──────────────────────────────────────────────────────────────

SPRecording::SPRecording(std::string isrc, std::string name, std::string version)
    : isrc(std::move(isrc)), name(std::move(name)), version(std::move(version)) {}

// ── Persistence ───────────────────────────────────────────────────────────────

bool SPRecording::save(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare(
        "INSERT OR REPLACE INTO recordings (isrc, name, version) VALUES (?, ?, ?)");
    sqlite3_bind_text(stmt, 1, isrc.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    if (!version.empty())
        sqlite3_bind_text(stmt, 3, version.c_str(), -1, SQLITE_TRANSIENT);
    else
        sqlite3_bind_null(stmt, 3);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool SPRecording::remove(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare("DELETE FROM recordings WHERE isrc = ?");
    sqlite3_bind_text(stmt, 1, isrc.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

// ── Deserialise ───────────────────────────────────────────────────────────────

SPRecording SPRecording::fromStatement(sqlite3_stmt* stmt) {
    auto text = [&](int col) -> std::string {
        const char* t = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
        return t ? t : "";
    };
    return SPRecording(text(0), text(1), text(2));
}

// ── Queries ───────────────────────────────────────────────────────────────────

std::optional<SPRecording> SPRecording::find(SPDBConnection& conn, const std::string& isrc) {
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT isrc, name, version FROM recordings WHERE isrc = ?");
    sqlite3_bind_text(stmt, 1, isrc.c_str(), -1, SQLITE_TRANSIENT);
    std::optional<SPRecording> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = fromStatement(stmt);
    sqlite3_finalize(stmt);
    return result;
}

std::vector<SPRecording> SPRecording::findAll(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT isrc, name, version FROM recordings ORDER BY name");
    std::vector<SPRecording> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}
