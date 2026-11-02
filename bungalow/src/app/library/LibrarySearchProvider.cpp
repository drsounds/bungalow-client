#include "app/library/LibrarySearchProvider.h"

#include "app/AppContext.h"

#include <sqlite3.h>

namespace space {
namespace app {
namespace {

constexpr int kLimit = 25;

constexpr char kQuery[] = R"(
    SELECT
        t.id,
        rec.name,
        COALESCE(GROUP_CONCAT(DISTINCT a.name), ''),
        COALESCE(rel.name, '')
    FROM saves s
    JOIN tracks             t   ON t.id            = s.track_id
    JOIN recordings         rec ON rec.isrc         = t.recording_id
    LEFT JOIN releases      rel ON rel.upc          = t.release_id
    LEFT JOIN recording_artists ra ON ra.recording_id = rec.isrc
    LEFT JOIN artists           a  ON a.isni          = ra.artist_id
    WHERE s.deleted_at IS NULL
      AND (rec.name LIKE ?1 ESCAPE '\'
           OR rel.name LIKE ?1 ESCAPE '\'
           OR EXISTS (SELECT 1
                      FROM recording_artists ra2
                      JOIN artists a2 ON a2.isni = ra2.artist_id
                      WHERE ra2.recording_id = rec.isrc
                        AND a2.name LIKE ?1 ESCAPE '\'))
    GROUP BY t.id
    ORDER BY rec.name
    LIMIT ?2
)";

/// %query% with LIKE wildcards in query matched literally.
std::string likePattern(const std::string& query) {
    std::string pattern = "%";
    for (char c : query) {
        if (c == '%' || c == '_' || c == '\\') pattern += '\\';
        pattern += c;
    }
    return pattern + "%";
}

std::string columnText(sqlite3_stmt* stmt, int column) {
    const unsigned char* text = sqlite3_column_text(stmt, column);
    return text ? reinterpret_cast<const char*>(text) : "";
}

} // namespace

std::vector<search::SearchResult> LibrarySearchProvider::search(const std::string& query) {
    std::vector<search::SearchResult> results;
    sqlite3* db = databaseHandle();
    if (!db) return results;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, kQuery, -1, &stmt, nullptr) != SQLITE_OK) return results;
    const std::string pattern = likePattern(query);
    sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, kLimit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        search::SearchResult result;
        result.kind = search::EntityKind::Track;
        result.title = columnText(stmt, 1);
        result.subtitle = columnText(stmt, 2);
        const std::string release = columnText(stmt, 3);
        if (!release.empty())
            result.subtitle += result.subtitle.empty() ? release : " \xE2\x80\x94 " + release;
        result.uri = "bungalow:track:" + std::to_string(sqlite3_column_int64(stmt, 0));
        result.source = getName();
        results.push_back(std::move(result));
    }
    sqlite3_finalize(stmt);
    return results;
}

} // namespace app
} // namespace space
