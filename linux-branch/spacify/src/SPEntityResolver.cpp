#include "SPEntityResolver.h"

#include <vector>
#include <string>
#include <utility>

// ── URI helpers ───────────────────────────────────────────────────────────────

// static
std::string SPEntityResolver::makeUri(const std::string& tableName, const std::string& uuid)
{
    return "spacify:" + tableName + ":" + uuid;
}

// static
bool SPEntityResolver::parseUri(const std::string& uri,
                                 std::string& tableName,
                                 std::string& uuid)
{
    // Must start with "spacify:"
    static const std::string kScheme = "spacify:";
    if (uri.rfind(kScheme, 0) != 0) return false;

    std::string rest = uri.substr(kScheme.size()); // "table:<name>:<uuid>"  or  "<name>:<uuid>"

    // Strip optional "table:" long prefix
    static const std::string kTablePrefix = "table:";
    if (rest.rfind(kTablePrefix, 0) == 0)
        rest = rest.substr(kTablePrefix.size());  // "<name>:<uuid>"

    // rest must be "<tableName>:<uuid>" — split at last ':'
    size_t colon = rest.rfind(':');
    if (colon == std::string::npos || colon == 0) return false;

    tableName = rest.substr(0, colon);
    uuid      = rest.substr(colon + 1);
    return !tableName.empty() && !uuid.empty();
}

// ── Public API ────────────────────────────────────────────────────────────────

// static
SPEntity SPEntityResolver::resolve(sqlite3* db, const std::string& uri)
{
    std::string tableName, uuid;
    if (!parseUri(uri, tableName, uuid)) return {};
    return loadFromDb(db, tableName, uuid);
}

// ── Internal DB lookup ────────────────────────────────────────────────────────

// static
SPEntity SPEntityResolver::loadFromDb(sqlite3* db,
                                       const std::string& tableName,
                                       const std::string& uuid)
{
    SPEntity entity;
    entity.type = tableName;
    entity.id   = uuid;
    entity.uri  = makeUri(tableName, uuid);

    if (!db || tableName.empty() || uuid.empty()) return entity;

    // ── 1. Load column definitions from db_tables ─────────────────────────────
    std::vector<std::pair<std::string,std::string>> columns;  // {name, type}
    {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db,
            "SELECT columns FROM db_tables WHERE name = ? LIMIT 1;",
            -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, tableName.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                const unsigned char* c = sqlite3_column_text(stmt, 0);
                std::string encoded = c ? reinterpret_cast<const char*>(c) : "";
                // Decode "name:type|name:type|..."
                size_t pos = 0;
                while (pos <= encoded.size()) {
                    size_t pipe  = encoded.find('|', pos);
                    std::string token = (pipe == std::string::npos)
                        ? encoded.substr(pos) : encoded.substr(pos, pipe - pos);
                    size_t sep = token.find(':');
                    if (sep != std::string::npos)
                        columns.emplace_back(token.substr(0, sep), token.substr(sep + 1));
                    if (pipe == std::string::npos) break;
                    pos = pipe + 1;
                }
            }
            sqlite3_finalize(stmt);
        }
    }
    if (columns.empty()) return entity; // table not registered

    // ── 2. Load the row ───────────────────────────────────────────────────────
    std::string sql = "SELECT * FROM user_" + tableName + " WHERE id=? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        return entity;

    sqlite3_bind_text(stmt, 1, uuid.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        entity.valid = true;
        int ncols = sqlite3_column_count(stmt);

        // col 0 = id, cols 1..N = user columns (parallel to columns[])
        std::string firstColValue;
        for (int i = 1; i < ncols && (i - 1) < static_cast<int>(columns.size()); ++i) {
            const unsigned char* t = sqlite3_column_text(stmt, i);
            std::string val = t ? reinterpret_cast<const char*>(t) : "";
            const std::string& colName = columns[i - 1].first;

            if (i == 1) firstColValue = val;  // always remember first value

            if (colName == "name")        entity.name        = val;
            if (colName == "image_url")   entity.imageUrl    = val;
            if (colName == "description") entity.description = val;
        }

        // Fallback: if no column named "name", use first column value
        if (entity.name.empty()) entity.name = firstColValue;
    }
    sqlite3_finalize(stmt);
    return entity;
}
