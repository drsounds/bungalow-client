#include "SPArtist.h"
#include "SPDBConnection.h"
#include <utility>

// ── Constructors ──────────────────────────────────────────────────────────────

SPArtist::SPArtist(std::string isni, std::string name)
    : isni(std::move(isni)), name(std::move(name)) {}

// ── Persistence ───────────────────────────────────────────────────────────────

bool SPArtist::save(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare(
        "INSERT OR REPLACE INTO artists (isni, name) VALUES (?, ?)");
    sqlite3_bind_text(stmt, 1, isni.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool SPArtist::remove(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare("DELETE FROM artists WHERE isni = ?");
    sqlite3_bind_text(stmt, 1, isni.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

// ── Deserialise ───────────────────────────────────────────────────────────────

SPArtist SPArtist::fromStatement(sqlite3_stmt* stmt) {
    auto text = [&](int col) -> std::string {
        const char* t = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
        return t ? t : "";
    };
    return SPArtist(text(0), text(1));
}

// ── Queries ───────────────────────────────────────────────────────────────────

std::optional<SPArtist> SPArtist::find(SPDBConnection& conn, const std::string& isni) {
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT isni, name FROM artists WHERE isni = ?");
    sqlite3_bind_text(stmt, 1, isni.c_str(), -1, SQLITE_TRANSIENT);
    std::optional<SPArtist> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = fromStatement(stmt);
    sqlite3_finalize(stmt);
    return result;
}

std::vector<SPArtist> SPArtist::findAll(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare("SELECT isni, name FROM artists ORDER BY name");
    std::vector<SPArtist> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}
