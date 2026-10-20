#pragma once

#include "../../space/src/SPTabBarViewElement.h"

#include <string>

namespace space {
class SPTableElement;
class SPObject;
class SPEventArgs;
class PlayQueueDataSource;

namespace views {

/**
 * PlayQueueView  —  right-sidebar panel
 *
 * Displays the current play queue. Subscribes to "queue:changed" and
 * "playback:changed" on BungalowApp's playback observable so it stays
 * in sync without requiring manual reload() calls.
 *
 * Double-clicking a row plays that entry via BungalowApp::playQueueAtIndex().
 */
class PlayQueueView : public SPTabBarViewElement {
public:
    PlayQueueView();
    explicit PlayQueueView(SPElement* parent);
    ~PlayQueueView() override;

    std::string getType() override { return "playqueueview"; }

private:
    int reload(SPObject* sender, SPEventArgs* args);
    int onRowDblClicked(SPObject* sender, SPEventArgs* args);

    SPTableElement*      m_table      = nullptr;
    PlayQueueDataSource* m_dataSource = nullptr;
};

} // namespace views
} // namespace space
