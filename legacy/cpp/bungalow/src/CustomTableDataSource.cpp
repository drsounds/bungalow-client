#include "CustomTableDataSource.h"
#include "SPEntityResolver.h"

namespace space {

// ── reload ────────────────────────────────────────────────────────────────────

void CustomTableDataSource::reload(sqlite3* db, const std::string& tableName)
{
    m_db   = db;
    m_def  = CustomTableDef{};
    m_rows.clear();
    if (!db || tableName.empty()) return;
    loadFromDb(db, tableName);
}

// ── SPTableDataSource interface ───────────────────────────────────────────────

int CustomTableDataSource::getColumnCount()
{
    return static_cast<int>(m_def.columns.size());
}

int CustomTableDataSource::getRowCount()
{
    return static_cast<int>(m_rows.size());
}

std::string CustomTableDataSource::getTextForColumn(int column)
{
    if (column < 0 || column >= static_cast<int>(m_def.columns.size())) return "";
    return m_def.columns[column].first;
}

std::string CustomTableDataSource::getTextForCell(int column, int row)
{
    if (row < 0 || row >= static_cast<int>(m_rows.size())) return "";
    if (column < 0 || column >= static_cast<int>(m_rows[row].values.size())) return "";
    return m_rows[row].values[column];
}

std::string CustomTableDataSource::getRowUuidForRow(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_rows.size())) return "";
    return m_rows[row].id;
}

// ── Data loading ──────────────────────────────────────────────────────────────

void CustomTableDataSource::loadFromDb(sqlite3* db, const std::string& tableName)
{
    // Load column definitions from db_tables
    {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db,
            "SELECT id, name, columns FROM db_tables WHERE name = ? LIMIT 1;",
            -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, tableName.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                m_def.id   = sqlite3_column_int(stmt, 0);
                const unsigned char* n = sqlite3_column_text(stmt, 1);
                m_def.name = n ? reinterpret_cast<const char*>(n) : "";
                const unsigned char* c = sqlite3_column_text(stmt, 2);
                std::string encoded = c ? reinterpret_cast<const char*>(c) : "";
                // Decode "name:type|name:type|..."
                size_t pos = 0;
                while (pos <= encoded.size()) {
                    size_t pipe  = encoded.find('|', pos);
                    std::string token = (pipe == std::string::npos)
                        ? encoded.substr(pos) : encoded.substr(pos, pipe - pos);
                    size_t colon = token.find(':');
                    if (colon != std::string::npos)
                        m_def.columns.emplace_back(token.substr(0, colon),
                                                   token.substr(colon + 1));
                    if (pipe == std::string::npos) break;
                    pos = pipe + 1;
                }
            }
            sqlite3_finalize(stmt);
        }
    }
    if (m_def.name.empty()) return; // table not found in meta

    // Load rows from the physical table
    std::string sql = "SELECT * FROM user_" + tableName + " ORDER BY id;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return;

    int colCount = sqlite3_column_count(stmt);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        CustomRow r;
        const unsigned char* idTxt = sqlite3_column_text(stmt, 0);
        r.id = idTxt ? reinterpret_cast<const char*>(idTxt) : "";
        for (int i = 1; i < colCount && (i-1) < static_cast<int>(m_def.columns.size()); ++i) {
            const unsigned char* t = sqlite3_column_text(stmt, i);
            std::string val = t ? reinterpret_cast<const char*>(t) : "";

            // Resolve BELONGS_TO columns to entity names for display
            const std::string& colType = m_def.columns[i - 1].second;
            if (colType.rfind("BELONGS_TO:", 0) == 0 && m_db && !val.empty()) {
                SPEntity ent = SPEntityResolver::resolve(m_db, val);
                if (ent.valid && !ent.name.empty())
                    val = ent.name;
            }

            r.values.push_back(std::move(val));
        }
        m_rows.push_back(std::move(r));
    }
    sqlite3_finalize(stmt);
}

} // namespace space
