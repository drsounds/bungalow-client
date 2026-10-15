#pragma once
#include "../../space/src/SPTableDataSource.h"
#include "DownloadManager.h"

#include <vector>

namespace space {

/// Table data source that filters DownloadManager items by status group.
class DownloadsDataSource : public SPTableDataSource {
public:
    enum class Filter { Ongoing, Completed };

    DownloadsDataSource(DownloadManager* dm, Filter filter);

    // Refresh snapshot from DownloadManager (call on update)
    void refresh();

    // SPTableDataSource overrides
    int    getColumnCount() override { return 4; }
    int    getRowCount()    override;
    string getTextForColumn(int column) override;
    string getTextForCell(int column, int row) override;
    float  getProgressForRow(int row) override;

    // For cancel action: return item id for a row
    int64_t getItemIdForRow(int row) const;

private:
    DownloadManager*          m_dm;
    Filter                    m_filter;
    std::vector<DownloadItem> m_snapshot;  // local copy updated on refresh()
};

} // namespace space
