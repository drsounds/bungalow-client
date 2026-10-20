#pragma once
#include "SPTabBarViewElement.h"
#include "SPTableElement.h"
#include "DownloadsDataSource.h"

namespace space {
namespace views {

class DownloadsView : public SPTabBarViewElement {
public:
    DownloadsView();
    DownloadsView(SPElement* parent, DownloadManager* dm);
    ~DownloadsView() override;

    bool acceptsUri(const std::string& uri) override;
    void navigate(const std::string& uri) override;
    void setHash(const std::string& hash) override;

    // Refresh both tables (called by DownloadManager's update callback)
    void refresh();

private:
    DownloadManager*     m_dm            = nullptr;
    SPTableElement*    m_ongoingTable    = nullptr;
    SPTableElement*    m_completedTable  = nullptr;
    DownloadsDataSource* m_ongoingDS     = nullptr;
    DownloadsDataSource* m_completedDS   = nullptr;

    int onOngoingRowDblClicked(SPObject* sender, SPEventArgs* args);
    int onCompletedRowDblClicked(SPObject* sender, SPEventArgs* args);
};

} // namespace views
} // namespace space
