#pragma once

#include "../../space/src/SPTableDataSource.h"
#include "Database.h"

#include <sqlite3.h>
#include <string>
#include <vector>

namespace space {

/**
 * CustomTableDataSource
 *
 * Backs a user-defined table view.  Columns and rows are driven entirely
 * by the CustomTableDef fetched from db_tables.
 *
 * Call reload(db, tableName) after any row mutation to refresh the view.
 */
class CustomTableDataSource : public SPTableDataSource {
public:
    CustomTableDataSource() = default;
    ~CustomTableDataSource() override = default;

    /** Load (or reload) data for the given table. */
    void reload(sqlite3* db, const std::string& tableName);

    // ── SPTableDataSource interface ───────────────────────────────────────────
    int         getColumnCount()                    override;
    int         getRowCount()                       override;
    std::string getTextForColumn(int column)        override;
    std::string getTextForCell  (int column, int row) override;
    int         getTrackIdForRow(int row)           override { return 0; } // UUID PK; not an int
    bool        isRowPlayable   (int /*row*/)       override { return false; }

    /** Returns the UUID primary key for the given row index, or "" if out of range. */
    std::string getRowUuidForRow(int row) const;

    const CustomTableDef& getTableDef() const { return m_def; }

private:
    sqlite3*               m_db  = nullptr;  // retained for BELONGS_TO resolution
    CustomTableDef           m_def;
    std::vector<CustomRow>   m_rows;

    void loadFromDb(sqlite3* db, const std::string& tableName);
};

} // namespace space
