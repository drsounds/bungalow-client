#pragma once

#include "PlayQueueEntry.h"
#include "SPTableDataSource.h"

#include <sqlite3.h>
#include <string>
#include <vector>

namespace space {

struct LocalFileRow {
    std::string name;
    std::string version;
    std::string artists;
    std::string album;
    std::string duration; // formatted "m:ss" or empty
    std::string path;
};

/**
 * LocalFilesDataSource
 *
 * Queries the audio_files table (joined to files for the path).
 * Columns: Name | Version | Artists | Album | Duration
 */
class LocalFilesDataSource : public SPTableDataSource {
public:
    explicit LocalFilesDataSource(sqlite3* db);
    ~LocalFilesDataSource() override = default;

    int         getColumnCount()                    override { return 5; }
    int         getRowCount()                       override;
    std::string getTextForCell(int column, int row) override;
    std::string getTextForColumn(int column)        override;

    void reload(sqlite3* db);

    /// Returns the file path for a given row index, or empty string if invalid.
    std::string getFilePathForRow(int row) const;

    /// Build a PlayQueueEntry for every row using file:// URIs. All entries are playable.
    std::vector<PlayQueueEntry> buildQueueEntries() const;

private:
    std::vector<LocalFileRow> m_rows;
    void load(sqlite3* db);
};

} // namespace space
