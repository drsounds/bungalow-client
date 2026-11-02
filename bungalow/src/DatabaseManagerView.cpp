#include "DatabaseManagerView.h"

#include "SPSectionElement.h"
#include "SPPlaybackEventArgs.h"
#include "SPTabBarElement.h"
#include "SPTabBarViewElement.h"
#include "SPTabStackElement.h"
#include "SPTableElement.h"

#include "MainElement.h"
#include "PlatformUi.h"
#include "BungalowApp.h"

#include <regex>

namespace space {

// ── DbTableListDataSource
// ─────────────────────────────────────────────────────

DbTableListDataSource::DbTableListDataSource(sqlite3 *db) {
	if (db)
		load(db);
}

void DbTableListDataSource::reload(sqlite3 *db) {
	m_tables.clear();
	if (db)
		load(db);
}

void DbTableListDataSource::load(sqlite3 *db) {
	sqlite3_stmt *stmt = nullptr;
	if (sqlite3_prepare_v2(db,
						   "SELECT id, name, columns, display_name FROM "
						   "db_tables ORDER BY name;",
						   -1, &stmt, nullptr) != SQLITE_OK)
		return;
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		CustomTableDef d;
		d.id = sqlite3_column_int(stmt, 0);
		const unsigned char *n = sqlite3_column_text(stmt, 1);
		d.name = n ? reinterpret_cast<const char *>(n) : "";
		const unsigned char *c = sqlite3_column_text(stmt, 2);
		std::string encoded = c ? reinterpret_cast<const char *>(c) : "";
		const unsigned char *dn = sqlite3_column_text(stmt, 3);
		d.displayName = dn ? reinterpret_cast<const char *>(dn) : "";
		int colCount = 0;
		if (!encoded.empty()) {
			colCount = 1;
			for (char ch : encoded)
				if (ch == '|')
					++colCount;
		}
		// Store column count in a dummy way: synthesize minimal column pairs
		// (we only need the count for the "Columns" display cell)
		for (int i = 0; i < colCount; ++i)
			d.columns.emplace_back(std::to_string(i), "");
		m_tables.push_back(std::move(d));
	}
	sqlite3_finalize(stmt);
}

int DbTableListDataSource::getRowCount() {
	return static_cast<int>(m_tables.size());
}

std::string DbTableListDataSource::getTextForColumn(int column) {
	if (column == 0)
		return "Table Name";
	if (column == 1)
		return "Columns";
	return "";
}

std::string DbTableListDataSource::getTextForCell(int column, int row) {
	if (row < 0 || row >= static_cast<int>(m_tables.size()))
		return "";
	const auto &t = m_tables[row];
	if (column == 0)
		return t.displayName.empty() ? t.name : t.displayName;
	if (column == 1)
		return std::to_string(t.columns.size());
	return "";
}

std::string DbTableListDataSource::getTableName(int row) const {
	if (row < 0 || row >= static_cast<int>(m_tables.size()))
		return "";
	return m_tables[row].name;
}

// ── DatabaseManagerView
// ───────────────────────────────────────────────────────

namespace views {

DatabaseManagerView::DatabaseManagerView() :
	SPTabBarViewElement() {}

DatabaseManagerView::DatabaseManagerView(SPElement *parent, sqlite3 *db) :
	SPTabBarViewElement(parent) {
	m_db = db;

	auto *section = new SPSectionElement(this->tabStack);
	section->setId("overview");
	section->setStringAttribute("title", "Database Manager");
	section->setFlex(1);

	m_dataSource = new DbTableListDataSource(db);
	m_table = new SPTableElement(section, m_dataSource);
	m_table->fullView = true;
	section->appendChild(m_table);
	m_table->setFlex(1);

	// Double-click → navigate to bungalow:table:<name>
	m_table->addEventListener(
		"rowdblclicked",
		reinterpret_cast<SPCallback>(&DatabaseManagerView::onRowDblClicked),
		this, m_table);

	// Right-click → Open / Delete
	m_table->addEventListener(
		"rowrightclicked",
		reinterpret_cast<SPCallback>(&DatabaseManagerView::onRowRightClicked),
		this, m_table);

	this->tabStack->appendChild(section);
	this->tabBar->addTab("overview", "Database Manager", "bungalow:database");
	this->tabStack->setTabId("overview");

	this->tabBar->addToolButton("new", "+ New Table");
	this->tabBar->addEventListener(
		"toolbuttonclicked",
		reinterpret_cast<SPCallback>(&DatabaseManagerView::onToolButtonClicked),
		this, this->tabBar);
}

DatabaseManagerView::~DatabaseManagerView() {}

bool DatabaseManagerView::acceptsUri(const std::string &uri) {
	return uri == "bungalow:database";
}

void DatabaseManagerView::navigate(const std::string & /*uri*/) {
	reload();
}

void DatabaseManagerView::setHash(const std::string &hash) {
	SPTabBarViewElement::setHash(hash.empty() ? "overview" : hash);
}

void DatabaseManagerView::reload() {
	if (m_dataSource && m_db) {
		m_dataSource->reload(m_db);
		if (m_table)
			m_table->invalidate();
	}
}

// The sidebar lists one item per custom table, so rebuild it after changes.
void DatabaseManagerView::rebuildNavigation() {
	for (SPElement *e = getParentElement(); e; e = e->getParentElement()) {
		if (auto *main = dynamic_cast<MainElement *>(e)) {
			main->rebuildNavItems();
			return;
		}
	}
}

int DatabaseManagerView::onRowDblClicked(SPObject * /*sender*/,
										 SPEventArgs *args) {
	auto *rowArgs = dynamic_cast<SPTableRowEventArgs *>(args);
	if (!rowArgs || !m_dataSource)
		return 0;

	std::string name = m_dataSource->getTableName(rowArgs->getRow());
	if (name.empty())
		return 0;

	if (auto *app = BungalowApp::current())
		app->navigate("bungalow:table:" + name);
	return 0;
}

int DatabaseManagerView::onRowRightClicked(SPObject * /*sender*/,
										   SPEventArgs *args) {
	auto *rowArgs = dynamic_cast<SPTableRowEventArgs *>(args);
	if (!rowArgs || !m_dataSource || !m_db)
		return 0;

	const std::string name = m_dataSource->getTableName(rowArgs->getRow());
	if (name.empty())
		return 0;

	enum { ID_OPEN = 10300, ID_DELETE };
	const int sel = platform::showPopupMenu({ { ID_OPEN, "Open" }, { ID_DELETE, "Delete" } });

	if (sel == ID_OPEN) {
		if (auto *app = BungalowApp::current())
			app->navigate("bungalow:table:" + name);
	} else if (sel == ID_DELETE) {
		if (!platform::showConfirmDialog("Confirm Delete",
				"Delete table '" + name + "'? All rows will be permanently lost."))
			return 0;

		auto *app = BungalowApp::current();
		if (!app || !app->getDatabase())
			return 0;
		app->getDatabase()->deleteCustomTable(name);
		reload();
		rebuildNavigation();
	}
	return 0;
}

int DatabaseManagerView::onToolButtonClicked(SPObject * /*sender*/,
											 SPEventArgs *args) {
	auto *btnArgs = dynamic_cast<space::TabBarToolButtonEventArgs *>(args);
	if (!btnArgs || btnArgs->buttonId != "new" || !m_db)
		return 0;

	if (platform::showCreateTableDialog(m_db)) {
		reload();
		rebuildNavigation();
	}
	return 0;
}

} // namespace views
} // namespace space
