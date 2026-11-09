#include "service/ResolutionStore.h"

namespace space {
namespace service {

namespace {

std::string columnText(sqlite3_stmt* stmt, int col) {
    const unsigned char* text = sqlite3_column_text(stmt, col);
    return text ? reinterpret_cast<const char*>(text) : std::string();
}

void bindText(sqlite3_stmt* stmt, int index, const std::string& value) {
    if (value.empty())
        sqlite3_bind_null(stmt, index);
    else
        sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_TRANSIENT);
}

} // namespace

std::optional<ResolutionStore::Pick> ResolutionStore::find(const PlayRequest& request) const {
    if (!m_db) return std::nullopt;
    // ?1 = 0 never matches a track (ids start at 1), so a request without a
    // library track is found by key alone.
    static const char* kSql =
        "SELECT service_id, match_isrc, match_title, match_artist, match_uri"
        " FROM music_service_tracks"
        " WHERE track_id = ?1 OR track_key = ?2"
        " ORDER BY track_id = ?1 DESC, id DESC LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, kSql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;
    sqlite3_bind_int64(stmt, 1, request.trackId);
    sqlite3_bind_text(stmt, 2, request.trackKey().c_str(), -1, SQLITE_TRANSIENT);

    std::optional<Pick> pick;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        pick = Pick{columnText(stmt, 0), columnText(stmt, 1), columnText(stmt, 2),
                    columnText(stmt, 3), columnText(stmt, 4)};
    }
    sqlite3_finalize(stmt);
    return pick;
}

bool ResolutionStore::save(const PlayRequest& request, const ServiceMatch& match) {
    if (!m_db || !match.service) return false;
    clear(request);

    static const char* kSql =
        "INSERT INTO music_service_tracks"
        " (track_id, track_key, service_id, match_isrc, match_title, match_artist, match_uri)"
        " VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, kSql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    if (request.trackId > 0)
        sqlite3_bind_int64(stmt, 1, request.trackId);
    else
        sqlite3_bind_null(stmt, 1);
    sqlite3_bind_text(stmt, 2, request.trackKey().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, match.service->getId().c_str(), -1, SQLITE_TRANSIENT);
    bindText(stmt, 4, match.candidate.byIsrc ? request.isrc : std::string());
    bindText(stmt, 5, request.title);
    bindText(stmt, 6, request.artist);
    bindText(stmt, 7, match.candidate.uri);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

void ResolutionStore::clear(const PlayRequest& request) {
    if (!m_db) return;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, "DELETE FROM music_service_tracks WHERE track_id = ?1 OR track_key = ?2;",
                           -1, &stmt, nullptr) != SQLITE_OK)
        return;
    sqlite3_bind_int64(stmt, 1, request.trackId);
    sqlite3_bind_text(stmt, 2, request.trackKey().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

} // namespace service
} // namespace space
