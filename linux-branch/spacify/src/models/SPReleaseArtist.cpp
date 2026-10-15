#include "SPReleaseArtist.h"
#include "SPDBConnection.h"
#include <utility>

// ── Constructors ──────────────────────────────────────────────────────────────

SPReleaseArtist::SPReleaseArtist(std::string releaseId, std::string artistId,
                                 int number, bool isPrimary)
    : releaseId(std::move(releaseId)), artistId(std::move(artistId)),
      number(number), isPrimaryArtist(isPrimary) {}

// ── Persistence ───────────────────────────────────────────────────────────────

bool SPReleaseArtist::save(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare(
        "INSERT OR REPLACE INTO release_artists "
        "(release_id, artist_id, number, is_primary) VALUES (?, ?, ?, ?)");
    sqlite3_bind_text(stmt, 1, releaseId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, artistId.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, number);
    sqlite3_bind_int(stmt, 4, isPrimaryArtist ? 1 : 0);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool SPReleaseArtist::remove(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare(
        "DELETE FROM release_artists WHERE release_id = ? AND artist_id = ?");
    sqlite3_bind_text(stmt, 1, releaseId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, artistId.c_str(),  -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

// ── Deserialise ───────────────────────────────────────────────────────────────

SPReleaseArtist SPReleaseArtist::fromStatement(sqlite3_stmt* stmt) {
    auto text = [&](int col) -> std::string {
        const char* t = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
        return t ? t : "";
    };
    return SPReleaseArtist(text(0), text(1),
                           sqlite3_column_int(stmt, 2),
                           sqlite3_column_int(stmt, 3) != 0);
}

// ── Queries ───────────────────────────────────────────────────────────────────

std::vector<SPReleaseArtist> SPReleaseArtist::findByRelease(
    SPDBConnection& conn, const std::string& releaseId)
{
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT release_id, artist_id, number, is_primary "
        "FROM release_artists WHERE release_id = ? ORDER BY number");
    sqlite3_bind_text(stmt, 1, releaseId.c_str(), -1, SQLITE_TRANSIENT);
    std::vector<SPReleaseArtist> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}

std::vector<SPReleaseArtist> SPReleaseArtist::findByArtist(
    SPDBConnection& conn, const std::string& artistId)
{
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT release_id, artist_id, number, is_primary "
        "FROM release_artists WHERE artist_id = ?");
    sqlite3_bind_text(stmt, 1, artistId.c_str(), -1, SQLITE_TRANSIENT);
    std::vector<SPReleaseArtist> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}

std::vector<SPReleaseArtist> SPReleaseArtist::findAll(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT release_id, artist_id, number, is_primary FROM release_artists");
    std::vector<SPReleaseArtist> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}
