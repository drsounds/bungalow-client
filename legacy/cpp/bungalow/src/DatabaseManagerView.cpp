#include "DatabaseManagerView.h"

#include "../../space/src/SPPageElement.h"
#include "../../space/src/SPPlaybackEventArgs.h"
#include "../../space/src/SPTabBarElement.h"
#include "../../space/src/SPTabBarViewElement.h"
#include "../../space/src/SPTabStackElement.h"
#include "../../space/src/SPTableElement.h"

#include "CreateTableDialog.h"
#include "BungalowApp.h"

#include <regex>
#include <wx/app.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>

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

	auto *page = new SPPageElement(this->tabStack);
	page->setId("overview");
	page->setStringAttribute("title", "Database Manager");
	page->setFlex(1);

	m_dataSource = new DbTableListDataSource(db);
	m_table = new SPTableElement(page, m_dataSource);
	m_table->fullView = true;
	page->appendChild(m_table);
	m_table->setFlex(1);

	// Double-click → navigate to bungalow:table:<name>
	m_table->addEventListener(
		"rowdblclicked",
		reinterpret_cast<SPCallback>(&DatabaseManagerView::onRowDblClicked),
		this, m_table);

	this->tabStack->appendChild(page);
	this->tabBar->addTab("overview", "Database Manager", "bungalow:database");
	this->tabStack->setTabId("overview");

	// Tool buttons
	this->tabBar->addToolButton("new", "+ New Table");
	this->tabBar->addToolButton("delete", "✕ Delete");
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
	if (m_dataSource && m_db) {
		m_dataSource->reload(m_db);
		if (m_table)
			m_table->invalidate();
	}
}

void DatabaseManagerView::setHash(const std::string &hash) {
	SPTabBarViewElement::setHash(hash.empty() ? "overview" : hash);
}

int DatabaseManagerView::onRowDblClicked(SPObject * /*sender*/,
										 SPEventArgs *args) {
	auto *rowArgs = dynamic_cast<SPTableRowEventArgs *>(args);
	if (!rowArgs || !m_dataSource)
		return 0;

	std::string name = m_dataSource->getTableName(rowArgs->getRow());
	if (name.empty())
		return 0;

	auto *app = dynamic_cast<BungalowApp *>(wxTheApp);
	if (app)
		app->navigate("bungalow:table:" + name);
	return 0;
}

int DatabaseManagerView::onToolButtonClicked(SPObject * /*sender*/,
											 SPEventArgs *args) {
	auto *btnArgs = dynamic_cast<space::TabBarToolButtonEventArgs *>(args);
	if (!btnArgs)
		return 0;

	wxWindow *parent = wxTheApp ? wxTheApp->GetTopWindow() : nullptr;
	if (!parent || !m_db)
		return 0;

	if (btnArgs->buttonId == "new") {
		CreateTableDialog dlg(parent, m_db);
		if (dlg.ShowModal() == wxID_OK) {
			if (m_dataSource)
				m_dataSource->reload(m_db);
			if (m_table)
				m_table->invalidate();

			// Also refresh the sidebar nav
			auto *app = dynamic_cast<BungalowApp *>(wxTheApp);
			if (app)
				app->navigate("bungalow:database");
		}
	} else if (btnArgs->buttonId == "delete") {
		// Find the selected row via the table element (use first-selected
		// heuristic)
		if (!m_dataSource)
			return 0;
		// We'll ask the user to right-click or we pop a dialog asking for name
		// For now: show an input dialog asking which table to delete
		wxString choice = wxGetTextFromUser(
			"Enter the exact table name to delete:", "Delete Table", "",
			parent);
		if (choice.IsEmpty())
			return 0;
		std::string name = choice.ToStdString();

		int confirm = wxMessageBox(
			"Delete table '" + name + "'? All rows will be permanently lost.",
			"Confirm Delete", wxYES_NO | wxICON_WARNING, parent);
		if (confirm != wxYES)
			return 0;

		// Delete via raw SQL (same logic as Database::deleteCustomTable)
		std::string ddl = "DROP TABLE IF EXISTS user_" + name + ";";
		sqlite3_exec(m_db, ddl.c_str(), nullptr, nullptr, nullptr);
		sqlite3_stmt *del = nullptr;
		if (sqlite3_prepare_v2(m_db, "DELETE FROM db_tables WHERE name = ?;",
							   -1, &del, nullptr) == SQLITE_OK) {
			sqlite3_bind_text(del, 1, name.c_str(), -1, SQLITE_TRANSIENT);
			sqlite3_step(del);
			sqlite3_finalize(del);
		}

		if (m_dataSource)
			m_dataSource->reload(m_db);
		if (m_table)
			m_table->invalidate();

		auto *app = dynamic_cast<BungalowApp *>(wxTheApp);
		if (app)
			app->navigate("bungalow:database");
	}
	return 0;
}

} // namespace views
} // namespace space
