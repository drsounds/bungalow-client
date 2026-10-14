#include "models/SPPlayableTrack.h"
#include "models/SPDBConnection.h"

// ── Constructors ──────────────────────────────────────────────────────────────

SPPlayableTrack::SPPlayableTrack(int64_t playableId, int64_t trackId)
    : playableId(playableId), trackId(trackId) {}

// ── Persistence ───────────────────────────────────────────────────────────────

bool SPPlayableTrack::save(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare(
        "INSERT OR IGNORE INTO playable_tracks (playable_id, track_id) VALUES (?, ?)");
    sqlite3_bind_int64(stmt, 1, playableId);
    sqlite3_bind_int64(stmt, 2, trackId);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool SPPlayableTrack::remove(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare(
        "DELETE FROM playable_tracks WHERE playable_id = ? AND track_id = ?");
    sqlite3_bind_int64(stmt, 1, playableId);
    sqlite3_bind_int64(stmt, 2, trackId);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

// ── Deserialise ───────────────────────────────────────────────────────────────

SPPlayableTrack SPPlayableTrack::fromStatement(sqlite3_stmt* stmt) {
    return SPPlayableTrack(sqlite3_column_int64(stmt, 0),
                           sqlite3_column_int64(stmt, 1));
}

// ── Queries ───────────────────────────────────────────────────────────────────

std::vector<SPPlayableTrack> SPPlayableTrack::findByPlayable(SPDBConnection& conn,
                                                              int64_t playableId)
{
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT playable_id, track_id FROM playable_tracks WHERE playable_id = ?");
    sqlite3_bind_int64(stmt, 1, playableId);
    std::vector<SPPlayableTrack> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}

std::vector<SPPlayableTrack> SPPlayableTrack::findByTrack(SPDBConnection& conn,
                                                           int64_t trackId)
{
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT playable_id, track_id FROM playable_tracks WHERE track_id = ?");
    sqlite3_bind_int64(stmt, 1, trackId);
    std::vector<SPPlayableTrack> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}

std::vector<SPPlayableTrack> SPPlayableTrack::findAll(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT playable_id, track_id FROM playable_tracks");
    std::vector<SPPlayableTrack> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}
