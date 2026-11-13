#include "CustomTableView.h"

#include "SPSectionElement.h"
#include "SPPlaybackEventArgs.h"
#include "SPTabBarElement.h"
#include "SPTabBarViewElement.h"
#include "SPTabStackElement.h"
#include "SPTableElement.h"

#include "PlatformUi.h"
#include "BungalowApp.h"

#include <regex>

namespace space {
namespace views {

CustomTableView::CustomTableView() :
	SPTabBarViewElement() {}

CustomTableView::CustomTableView(SPElement *parent, sqlite3 *db) :
	SPTabBarViewElement(parent) {
	m_db = db;

	// We build a placeholder section — content is populated on navigate()
	auto *section = new SPSectionElement(this->tabStack);
	section->setId("overview");
	section->setStringAttribute("title", "Table");
	section->setFlex(1);

	m_dataSource = new CustomTableDataSource();
	m_table = new SPTableElement(section, m_dataSource);
	m_table->fullView = true;
	section->appendChild(m_table);
	m_table->setFlex(1);

	// Double-click → navigate to entry detail view
	m_table->addEventListener(
		"rowdblclicked",
		reinterpret_cast<SPCallback>(&CustomTableView::onRowDblClicked), this,
		m_table);

	// Right-click for Edit / Delete
	m_table->addEventListener(
		"rowrightclicked",
		reinterpret_cast<SPCallback>(&CustomTableView::onRowRightClicked), this,
		m_table);

	// Delete removes the selected rows and Ctrl+Z puts them back.
	m_table->setDeleteHandler(
		[this](const std::vector<int> &rows) { return deleteRows(rows); },
		[](const std::vector<int> &rows) {
			return platform::showConfirmDialog(
				"Confirm Delete",
				rows.size() == 1 ? "Delete this row?"
								 : "Delete " + std::to_string(rows.size()) + " rows?");
		});

	this->tabStack->appendChild(section);
	this->tabBar->addTab("overview", "Table", "bungalow:table:");
	this->tabStack->setTabId("overview");

	// + tool button
	this->tabBar->addToolButton("add", "+");
	this->tabBar->addEventListener(
		"toolbuttonclicked",
		reinterpret_cast<SPCallback>(&CustomTableView::onToolButtonClicked),
		this, this->tabBar);
}

CustomTableView::~CustomTableView() {}

bool CustomTableView::acceptsUri(const std::string &uri) {
	// Match "bungalow:table:<name>" but NOT "bungalow:table:<name>:<id>"
	// The entry view handles the 4-segment form.
	static const std::string kPrefix = "bungalow:table:";
	if (uri.rfind(kPrefix, 0) != 0)
		return false;
	std::string rest = uri.substr(kPrefix.size());
	// rest must be non-empty and contain no second colon
	return !rest.empty() && rest.find(':') == std::string::npos;
}

void CustomTableView::navigate(const std::string &uri) {
	// Extract table name from "bungalow:table:<name>"
	static const std::string kPrefix = "bungalow:table:";
	if (uri.rfind(kPrefix, 0) != 0)
		return;
	const std::string tableName = uri.substr(kPrefix.size());
	if (tableName != m_tableName && m_table)
		m_table->clearUndoHistory();   // undo is per table
	m_tableName = tableName;

	if (m_dataSource && m_db) {
		m_dataSource->reload(m_db, m_tableName);
		if (m_table)
			m_table->invalidate();
	}
}

void CustomTableView::setHash(const std::string &hash) {
	SPTabBarViewElement::setHash(hash.empty() ? "overview" : hash);
}

int CustomTableView::onToolButtonClicked(SPObject * /*sender*/,
										 SPEventArgs *args) {
	auto *btnArgs = dynamic_cast<space::TabBarToolButtonEventArgs *>(args);
	if (!btnArgs || btnArgs->buttonId != "add")
		return 0;
	if (!m_db || m_tableName.empty())
		return 0;

	if (platform::showCustomRowDialog(m_db, m_tableName)) {
		if (m_dataSource)
			m_dataSource->reload(m_db, m_tableName);
		if (m_table)
			m_table->invalidate();
	}
	return 0;
}

int CustomTableView::onRowDblClicked(SPObject * /*sender*/, SPEventArgs *args) {
	auto *rowArgs = dynamic_cast<SPTableRowEventArgs *>(args);
	if (!rowArgs || !m_dataSource || m_tableName.empty())
		return 0;

	std::string uuid = m_dataSource->getRowUuidForRow(rowArgs->getRow());
	if (uuid.empty())
		return 0;

	if (auto *app = BungalowApp::current())
		app->navigate("bungalow:table:" + m_tableName + ":" + uuid);
	return 0;
}

std::function<void()> CustomTableView::deleteRows(const std::vector<int> &rows) {
	auto *app = BungalowApp::current();
	Database *db = app ? app->getDatabase() : nullptr;
	if (!db || !m_dataSource || m_tableName.empty())
		return {};

	const std::string table = m_tableName;
	const CustomTableDef def = m_dataSource->getTableDef();
	std::vector<CustomRow> removed;   // what undo has to put back
	for (int row : rows) {
		const std::string uuid = m_dataSource->getRowUuidForRow(row);
		CustomRow data = uuid.empty() ? CustomRow() : db->getCustomRow(table, uuid);
		if (!data.id.empty())
			removed.push_back(std::move(data));
	}
	if (removed.empty())
		return {};
	for (const CustomRow &row : removed)
		db->deleteCustomRow(table, row.id);
	reloadTable();

	return [this, table, def, removed] {
		auto *app = BungalowApp::current();
		Database *db = app ? app->getDatabase() : nullptr;
		if (!db)
			return;
		for (const CustomRow &row : removed)
			db->insertCustomRow(table, def, row.values, row.id);
		if (m_tableName == table)
			reloadTable();
	};
}

void CustomTableView::reloadTable() {
	if (m_dataSource && m_db)
		m_dataSource->reload(m_db, m_tableName);
	if (m_table)
		m_table->invalidate();
}

int CustomTableView::onRowRightClicked(SPObject * /*sender*/,
									   SPEventArgs *args) {
	auto *rowArgs = dynamic_cast<SPTableRowEventArgs *>(args);
	if (!rowArgs || !m_dataSource || !m_db)
		return 0;

	std::string uuid = m_dataSource->getRowUuidForRow(rowArgs->getRow());
	if (uuid.empty())
		return 0;

	enum { ID_EDIT = 10200, ID_DELETE };
	const int sel = platform::showPopupMenu({ { ID_EDIT, "Edit..." }, { ID_DELETE, "Delete" } });

	bool changed = false;
	if (sel == ID_EDIT) {
		changed = platform::showCustomRowDialog(m_db, m_tableName, uuid);
	} else if (sel == ID_DELETE) {
		auto *app = BungalowApp::current();
		if (app && app->getDatabase() &&
			platform::showConfirmDialog("Confirm", "Delete this row?")) {
			app->getDatabase()->deleteCustomRow(m_tableName, uuid);
			changed = true;
		}
	}

	if (changed) {
		if (m_dataSource)
			m_dataSource->reload(m_db, m_tableName);
		if (m_table)
			m_table->invalidate();
	}
	return 0;
}

} // namespace views
} // namespace space
