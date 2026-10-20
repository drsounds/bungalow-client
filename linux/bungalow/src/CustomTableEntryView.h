#pragma once

#include "../../space/src/SPViewElement.h"
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
 * CustomTableEntryView  —  bungalow:table:<tableName>:<rowId>
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
 * URI pattern:  bungalow:table:<tableName>:<rowId>
 *   where <rowId> is the INTEGER PRIMARY KEY of the row.
 */
class CustomTableEntryView : public SPViewElement {
public:
	CustomTableEntryView();
	explicit CustomTableEntryView(SPElement *parent, sqlite3 *db = nullptr);
	~CustomTableEntryView() override;

	std::string getType() override { return "customtableentryview"; }

	bool acceptsUri(const std::string &uri) override;
	void navigate(const std::string &uri) override;

private:
	sqlite3 *m_db = nullptr;
	std::string m_tableName;
	std::string m_rowId; // UUID4 primary key, empty = not set

	// ── UI slots rebuilt on each navigate() ──────────────────────────
	SPHeaderElement *m_header = nullptr; // hero (image + title)
	SPVBoxElement *m_body = nullptr;	 // scrollable field list

	/** Tear down and rebuild all child UI from current m_tableName / m_rowId.
	 */
	void rebuild();

	/** Load column defs + row values from SQLite. Returns false on failure. */
	bool loadData(CustomTableDef &outDef, std::vector<std::string> &outValues);
};

} // namespace views
} // namespace space
