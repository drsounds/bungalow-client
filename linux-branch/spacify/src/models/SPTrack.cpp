#include "SPTrack.h"
#include "SPDBConnection.h"

#include <utility>

// ── Constructors ──────────────────────────────────────────────────────────────

SPTrack::SPTrack(std::string recordingId, std::string releaseId)
    : recordingId(std::move(recordingId)), releaseId(std::move(releaseId)) {}

SPTrack::SPTrack(int64_t id, std::string recordingId, std::string releaseId)
    : id(id), recordingId(std::move(recordingId)), releaseId(std::move(releaseId)) {}

// ── Persistence ───────────────────────────────────────────────────────────────

bool SPTrack::save(SPDBConnection& conn) {
    if (!isPersisted()) {
        sqlite3_stmt* stmt = conn.prepare(
            "INSERT INTO tracks (recording_id, release_id) VALUES (?, ?)");
        sqlite3_bind_text(stmt, 1, recordingId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, releaseId.c_str(),   -1, SQLITE_TRANSIENT);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        if (rc == SQLITE_DONE)
            id = sqlite3_last_insert_rowid(conn.handle());
        return rc == SQLITE_DONE;
    } else {
        sqlite3_stmt* stmt = conn.prepare(
            "UPDATE tracks SET recording_id = ?, release_id = ? WHERE id = ?");
        sqlite3_bind_text(stmt, 1, recordingId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, releaseId.c_str(),   -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 3, id);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return rc == SQLITE_DONE;
    }
}

bool SPTrack::remove(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare("DELETE FROM tracks WHERE id = ?");
    sqlite3_bind_int64(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

// ── Deserialise ───────────────────────────────────────────────────────────────

SPTrack SPTrack::fromStatement(sqlite3_stmt* stmt) {
    auto text = [&](int col) -> std::string {
        const char* t = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
        return t ? t : "";
    };
    return SPTrack(sqlite3_column_int64(stmt, 0), text(1), text(2));
}

// ── Queries ───────────────────────────────────────────────────────────────────

std::optional<SPTrack> SPTrack::find(SPDBConnection& conn, int64_t id) {
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT id, recording_id, release_id FROM tracks WHERE id = ?");
    sqlite3_bind_int64(stmt, 1, id);
    std::optional<SPTrack> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = fromStatement(stmt);
    sqlite3_finalize(stmt);
    return result;
}

std::vector<SPTrack> SPTrack::findAll(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT id, recording_id, release_id FROM tracks");
    std::vector<SPTrack> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}

std::vector<SPTrack> SPTrack::findByRecording(SPDBConnection& conn,
                                               const std::string& recordingId)
{
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT id, recording_id, release_id FROM tracks WHERE recording_id = ?");
    sqlite3_bind_text(stmt, 1, recordingId.c_str(), -1, SQLITE_TRANSIENT);
    std::vector<SPTrack> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}

std::vector<SPTrack> SPTrack::findByRelease(SPDBConnection& conn,
                                             const std::string& releaseId)
{
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT id, recording_id, release_id FROM tracks WHERE release_id = ?");
    sqlite3_bind_text(stmt, 1, releaseId.c_str(), -1, SQLITE_TRANSIENT);
    std::vector<SPTrack> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}
