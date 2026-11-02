#include "app/database/DatabaseFeature.h"

#include "app/AppContext.h"

#include "CustomTableEntryTabBarView.h"
#include "CustomTableView.h"
#include "Database.h"
#include "DatabaseManagerView.h"

namespace space {
namespace app {

std::vector<SPViewElement*> DatabaseFeature::createViews(SPViewStackElement* viewStack) {
    sqlite3* db = databaseHandle();
    return {
        new views::DatabaseManagerView(viewStack, db),
        new views::CustomTableEntryTabBarView(viewStack, db),
        new views::CustomTableView(viewStack, db),
    };
}

std::vector<navigation::SidebarNode> DatabaseFeature::getSidebarNodes() const {
    navigation::SidebarNode manager("Table Manager", "bungalow:database");
    manager.setIconPath("icons/Document.svg").setExpanded(true);

    // One child item per user-defined table
    if (Database* db = database()) {
        for (const auto& table : db->getCustomTables()) {
            manager.addChild(navigation::SidebarNode(table.displayName.empty() ? table.name : table.displayName,
                                                     "bungalow:table:" + table.name)
                                 .setIconPath("icons/Document.svg"));
        }
    }

    navigation::SidebarNode section("DATABASE");
    section.addChild(manager);
    return {section};
}

} // namespace app
} // namespace space
