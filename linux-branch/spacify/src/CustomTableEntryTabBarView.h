#pragma once

#include "../../space/src/SPTabBarViewElement.h"
#include "CustomTableEntryView.h"
#include "Database.h"

#include <sqlite3.h>
#include <string>
#include <vector>

namespace space {
class SPHeaderElement;
class SPVBoxElement;
class SPTextElement;

namespace views {

/**
 * CustomTableEntryView  —  spacify:table:<tableName>:<rowId>
 *
 * Displays a single row of a user-defined table as a "node detail" page:
 *
 *   ┌─────────────────────────────────────────┐
 *   │  [icon]  <first-column value>  (large)  │  ← SPHeaderElement
 *   │          <table name> / <row id>        │
 *   ├─────────────────────────────────────────┤
 *   │  ColumnName1    value1                  │  ← field rows
 *   │  ColumnName2    value2                  │
 *   │  ...                                    │
 *   └─────────────────────────────────────────┘
 *
 * URI pattern:  spacify:table:<tableName>:<rowId>
 *   where <rowId> is the INTEGER PRIMARY KEY of the row.
 */
class CustomTableEntryTabBarView : public SPTabBarViewElement {
public:
	CustomTableEntryTabBarView();
	explicit CustomTableEntryTabBarView(SPElement *parent,
										sqlite3 *db = nullptr);
	~CustomTableEntryTabBarView() override;

	std::string getType() override { return "customtableentrytabbarview"; }

	bool acceptsUri(const std::string &uri) override;
	void navigate(const std::string &uri) override;

private:
	CustomTableEntryView *customTableEntryView;
	sqlite3 *m_db = nullptr;
	std::string m_tableName;
	std::string m_rowId; // UUID4 primary key, empty = not set
};

} // namespace views
} // namespace space
