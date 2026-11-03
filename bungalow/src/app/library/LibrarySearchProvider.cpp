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

/// One kind of library entity matched by name (LIKE ?1) or exact identifier
/// (= ?2). Columns: identifier, name, subtitle. Results open the entity's
/// library detail page, bungalow:library:<kind>:<identifier>.
struct EntityQuery {
    search::EntityKind kind;
    const char*        uriKind;
    const char*        sql;
};

constexpr EntityQuery kEntityQueries[] = {
    { search::EntityKind::Artist, "artist", R"(
        SELECT a.isni, a.name, ''
        FROM artists a
        WHERE a.name LIKE ?1 ESCAPE '\' OR a.isni = ?2
        ORDER BY a.name COLLATE NOCASE
        LIMIT ?3
    )" },
    { search::EntityKind::Release, "release", R"(
        SELECT rel.upc, rel.name,
               COALESCE((SELECT GROUP_CONCAT(a.name, ', ')
                         FROM release_artists ra JOIN artists a ON a.isni = ra.artist_id
                         WHERE ra.release_id = rel.upc), '')
        FROM releases rel
        WHERE rel.name LIKE ?1 ESCAPE '\' OR rel.upc = ?2
        ORDER BY rel.name COLLATE NOCASE
        LIMIT ?3
    )" },
    { search::EntityKind::Recording, "recording", R"(
        SELECT rec.isrc, rec.name,
               COALESCE((SELECT GROUP_CONCAT(a.name, ', ')
                         FROM recording_artists ra JOIN artists a ON a.isni = ra.artist_id
                         WHERE ra.recording_id = rec.isrc), '')
        FROM recordings rec
        WHERE rec.name LIKE ?1 ESCAPE '\' OR rec.isrc = ?2
        ORDER BY rec.name COLLATE NOCASE
        LIMIT ?3
    )" },
};

void searchEntities(sqlite3* db, const EntityQuery& q, const std::string& pattern,
                    const std::string& exact, const std::string& source,
                    std::vector<search::SearchResult>& results) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, q.sql, -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, exact.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, kLimit);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        search::SearchResult result;
        result.kind = q.kind;
        result.title = columnText(stmt, 1);
        result.subtitle = columnText(stmt, 2);
        result.uri = std::string("bungalow:library:") + q.uriKind + ":" + columnText(stmt, 0);
        result.source = source;
        results.push_back(std::move(result));
    }
    sqlite3_finalize(stmt);
}

} // namespace

std::vector<search::SearchResult> LibrarySearchProvider::search(const std::string& query) {
    std::vector<search::SearchResult> results;
    sqlite3* db = databaseHandle();
    if (!db) return results;

    const std::string pattern = likePattern(query);
    for (const auto& q : kEntityQueries)
        searchEntities(db, q, pattern, query, getName(), results);

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, kQuery, -1, &stmt, nullptr) != SQLITE_OK) return results;
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
