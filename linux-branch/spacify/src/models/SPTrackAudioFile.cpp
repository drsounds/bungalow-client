#include "SPTrackAudioFile.h"
#include "SPDBConnection.h"

// ── Constructors ──────────────────────────────────────────────────────────────

SPTrackAudioFile::SPTrackAudioFile(int64_t trackId, int64_t audioFileId)
    : trackId(trackId), audioFileId(audioFileId) {}

// ── Persistence ───────────────────────────────────────────────────────────────

bool SPTrackAudioFile::save(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare(
        "INSERT OR IGNORE INTO track_audio_files (track_id, audio_file_id) VALUES (?, ?)");
    sqlite3_bind_int64(stmt, 1, trackId);
    sqlite3_bind_int64(stmt, 2, audioFileId);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool SPTrackAudioFile::remove(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare(
        "DELETE FROM track_audio_files WHERE track_id = ? AND audio_file_id = ?");
    sqlite3_bind_int64(stmt, 1, trackId);
    sqlite3_bind_int64(stmt, 2, audioFileId);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

// ── Deserialise ───────────────────────────────────────────────────────────────

SPTrackAudioFile SPTrackAudioFile::fromStatement(sqlite3_stmt* stmt) {
    return SPTrackAudioFile(sqlite3_column_int64(stmt, 0),
                            sqlite3_column_int64(stmt, 1));
}

// ── Queries ───────────────────────────────────────────────────────────────────

std::vector<SPTrackAudioFile> SPTrackAudioFile::findByTrack(SPDBConnection& conn,
                                                             int64_t trackId)
{
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT track_id, audio_file_id FROM track_audio_files WHERE track_id = ?");
    sqlite3_bind_int64(stmt, 1, trackId);
    std::vector<SPTrackAudioFile> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}

std::vector<SPTrackAudioFile> SPTrackAudioFile::findByAudioFile(SPDBConnection& conn,
                                                                 int64_t audioFileId)
{
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT track_id, audio_file_id FROM track_audio_files WHERE audio_file_id = ?");
    sqlite3_bind_int64(stmt, 1, audioFileId);
    std::vector<SPTrackAudioFile> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}

std::vector<SPTrackAudioFile> SPTrackAudioFile::findAll(SPDBConnection& conn) {
    sqlite3_stmt* stmt = conn.prepare(
        "SELECT track_id, audio_file_id FROM track_audio_files");
    std::vector<SPTrackAudioFile> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(fromStatement(stmt));
    sqlite3_finalize(stmt);
    return result;
}
