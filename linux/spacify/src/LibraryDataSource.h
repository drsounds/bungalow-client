#pragma once

#include "PlayQueueEntry.h"
#include "../../space/src/SPTableDataSource.h"

#include <sqlite3.h>
#include <string>
#include <vector>

namespace space {

struct LibraryRow {
    std::string name;        // recording name
    std::string artists;     // comma-joined recording artist names
    std::string releaseName; // release name
    int         trackId  = 0;
    bool        playable = false; // true if at least one playable_tracks entry exists
};

/**
 * LibraryDataSource
 *
 * Queries the 'saves' table (non-deleted) and joins through:
 *   saves → tracks → recordings → releases
 *   recordings → recording_artists → artists   (for the Artists column)
 *
 * Columns exposed: Name | Artists | Release
 */
class LibraryDataSource : public SPTableDataSource {
public:
    explicit LibraryDataSource(sqlite3* db);
    ~LibraryDataSource() override = default;

    int         getColumnCount()                    override { return 3; }
    int         getRowCount()                       override;
    std::string getTextForCell(int column, int row) override;
    std::string getTextForColumn(int column)        override;
    int         getTrackIdForRow(int row)           override;
    bool        isRowPlayable(int row)              override;
    std::string getRowUri(int index)         override;
    /** Reload rows from the database (call after saves change). */
    void reload(sqlite3* db);

    /**
     * Build a PlayQueueEntry for every row in the current snapshot.
     * Resolves all track→URI mappings with a single SQL query.
     * Rows without a playable_tracks entry have playable=false and empty uri.
     */
    std::vector<PlayQueueEntry> buildQueueEntries(sqlite3* db) const;

private:
    std::vector<LibraryRow> m_rows;

    void load(sqlite3* db);
};

} // namespace space
