#include "SPRelease.h"
#include "SPDBConnection.h"
#include <utility>

// ── Constructors ──────────────────────────────────────────────────────────────

SPRelease::SPRelease(std::string upc, std::string name,
                     std::string releaseDate, int64_t labelId, std::string coverArt)
    : upc(std::move(upc)), name(std::move(name)),
      releaseDate(std::move(releaseDate)), labelId(labelId),
      coverArt(std::move(coverArt)) {}

// ── Persistence ───────────────────────────────────────────────────────────────

bool SPRelease::save(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare(
        "INSERT OR REPLACE INTO releases "
        "(upc, name, release_date, label_id, cover_art) VALUES (?, ?, ?, ?, ?)");
    sqlite3_bind_text(stmt, 1, upc.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    if (!releaseDate.empty())
        sqlite3_bind_text(stmt, 3, releaseDate.c_str(), -1, SQLITE_TRANSIENT);
    else
        sqlite3_bind_null(stmt, 3);
    if (labelId != 0)
        sqlite3_bind_int64(stmt, 4, labelId);
    else
        sqlite3_bind_null(stmt, 4);
    if (!coverArt.empty())
        sqlite3_bind_text(stmt, 5, coverArt.c_str(), -1, SQLITE_TRANSIENT);
    else
        sqlite3_bind_null(stmt, 5);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool SPRelease::remove(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare("DELETE FROM releases WHERE upc = ?");
    sqlite3_bind_text(stmt, 1, upc.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

// ── Deserialise ───────────────────────────────────────────────────────────────

SPRelease SPRelease::fromStatement(sqlite3_stmt* stmt) {
    auto text = [&](int col) -> std::string {
        const char* t = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
        return t ? t : "";
    };
    int64_t lId = (sqlite3_column_type(stmt, 3) != SQLITE_NULL)
                      ? sqlite3_column_int64(stmt, 3)
                      : 0;
    return SPRelease(text(0), text(1), text(2), lId, text(4));
}

// ── Queries ───────────────────────────────────────────────────────────────────

std::optional<SPRelease> SPRelease::find(SPDBConnection& conn, const std::string& upc) {
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT upc, name, release_date, label_id, cover_art FROM releases WHERE upc = ?");
    sqlite3_bind_text(stmt, 1, upc.c_str(), -1, SQLITE_TRANSIENT);
    std::optional<SPRelease> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = fromStatement(stmt);
    sqlite3_finalize(stmt);
    return result;
}

std::vector<SPRelease> SPRelease::findAll(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT upc, name, release_date, label_id, cover_art FROM releases ORDER BY name");
    std::vector<SPRelease> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}
