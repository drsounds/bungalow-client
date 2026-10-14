#include "models/SPRecordingArtist.h"
#include "models/SPDBConnection.h"
#include <utility>

// ── Constructors ──────────────────────────────────────────────────────────────

SPRecordingArtist::SPRecordingArtist(std::string recordingId, std::string artistId,
                                     bool isPrimary, int number)
    : recordingId(std::move(recordingId)), artistId(std::move(artistId)),
      isPrimaryArtist(isPrimary), number(number) {}

// ── Persistence ───────────────────────────────────────────────────────────────

bool SPRecordingArtist::save(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare(
        "INSERT OR REPLACE INTO recording_artists "
        "(recording_id, artist_id, is_primary, number) VALUES (?, ?, ?, ?)");
    sqlite3_bind_text(stmt, 1, recordingId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, artistId.c_str(),   -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, isPrimaryArtist ? 1 : 0);
    sqlite3_bind_int(stmt, 4, number);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool SPRecordingArtist::remove(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare(
        "DELETE FROM recording_artists WHERE recording_id = ? AND artist_id = ?");
    sqlite3_bind_text(stmt, 1, recordingId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, artistId.c_str(),   -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

// ── Deserialise ───────────────────────────────────────────────────────────────

SPRecordingArtist SPRecordingArtist::fromStatement(sqlite3_stmt* stmt) {
    auto text = [&](int col) -> std::string {
        const char* t = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
        return t ? t : "";
    };
    return SPRecordingArtist(text(0), text(1),
                             sqlite3_column_int(stmt, 2) != 0,
                             sqlite3_column_int(stmt, 3));
}

// ── Queries ───────────────────────────────────────────────────────────────────

std::vector<SPRecordingArtist> SPRecordingArtist::findByRecording(
    SPDBConnection& conn, const std::string& recordingId)
{
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT recording_id, artist_id, is_primary, number "
        "FROM recording_artists WHERE recording_id = ? ORDER BY number");
    sqlite3_bind_text(stmt, 1, recordingId.c_str(), -1, SQLITE_TRANSIENT);
    std::vector<SPRecordingArtist> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}

std::vector<SPRecordingArtist> SPRecordingArtist::findByArtist(
    SPDBConnection& conn, const std::string& artistId)
{
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT recording_id, artist_id, is_primary, number "
        "FROM recording_artists WHERE artist_id = ?");
    sqlite3_bind_text(stmt, 1, artistId.c_str(), -1, SQLITE_TRANSIENT);
    std::vector<SPRecordingArtist> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}

std::vector<SPRecordingArtist> SPRecordingArtist::findAll(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT recording_id, artist_id, is_primary, number FROM recording_artists");
    std::vector<SPRecordingArtist> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}
