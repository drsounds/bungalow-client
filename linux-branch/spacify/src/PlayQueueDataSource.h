#pragma once

#include "PlayQueueEntry.h"
#include "../../space/src/SPTableDataSource.h"

#include <vector>

namespace space {

/**
 * PlayQueueDataSource
 *
 * Provides the play queue (a vector of PlayQueueEntry) to an SPTableElement.
 * Two columns: Track name | Artist.
 * Unplayable entries are reflected through isRowPlayable().
 * The currently-playing row is reflected through isRowActive().
 */
class PlayQueueDataSource : public SPTableDataSource {
public:
    PlayQueueDataSource() = default;
    ~PlayQueueDataSource() override = default;

    int         getColumnCount()                    override { return 2; }
    int         getRowCount()                       override;
    std::string getTextForCell(int column, int row) override;
    std::string getTextForColumn(int column)        override;
    bool        isRowPlayable(int row)              override;
    bool        isRowActive(int index)              override;

    /** Replace the displayed entries and set which row is currently active (-1 = none). */
    void setEntries(const std::vector<PlayQueueEntry>& entries, int activeIndex);

private:
    std::vector<PlayQueueEntry> m_entries;
    int                         m_activeIndex = -1;
};

} // namespace space
