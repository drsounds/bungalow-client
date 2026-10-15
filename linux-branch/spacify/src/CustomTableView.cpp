#include "CustomTableView.h"

#include "../../space/src/SPPageElement.h"
#include "../../space/src/SPPlaybackEventArgs.h"
#include "../../space/src/SPTabBarElement.h"
#include "../../space/src/SPTabBarViewElement.h"
#include "../../space/src/SPTabStackElement.h"
#include "../../space/src/SPTableElement.h"

#include "CustomRowDialog.h"
#include "SpacifyApp.h"

#include <regex>
#include <wx/app.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>

namespace space {
namespace views {

CustomTableView::CustomTableView() :
	SPTabBarViewElement() {}

CustomTableView::CustomTableView(SPElement *parent, sqlite3 *db) :
	SPTabBarViewElement(parent) {
	m_db = db;

	// We build a placeholder section — content is populated on navigate()
	auto *page = new SPPageElement(this->tabStack);
	page->setId("overview");
	page->setStringAttribute("title", "Table");
	page->setFlex(1);

	m_dataSource = new CustomTableDataSource();
	m_table = new SPTableElement(page, m_dataSource);
	m_table->fullView = true;
	page->appendChild(m_table);
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

	this->tabStack->appendChild(page);
	this->tabBar->addTab("overview", "Table", "spacify:table:");
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
	// Match "spacify:table:<name>" but NOT "spacify:table:<name>:<id>"
	// The entry view handles the 4-segment form.
	static const std::string kPrefix = "spacify:table:";
	if (uri.rfind(kPrefix, 0) != 0)
		return false;
	std::string rest = uri.substr(kPrefix.size());
	// rest must be non-empty and contain no second colon
	return !rest.empty() && rest.find(':') == std::string::npos;
}

void CustomTableView::navigate(const std::string &uri) {
	// Extract table name from "spacify:table:<name>"
	static const std::string kPrefix = "spacify:table:";
	if (uri.rfind(kPrefix, 0) != 0)
		return;
	m_tableName = uri.substr(kPrefix.size());

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

	wxWindow *parent = wxTheApp ? wxTheApp->GetTopWindow() : nullptr;
	if (!parent || !m_db || m_tableName.empty())
		return 0;

	CustomRowDialog dlg(parent, m_db, m_tableName);
	if (dlg.ShowModal() == wxID_OK) {
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

	auto *app = dynamic_cast<SpacifyApp *>(wxTheApp);
	if (app)
		app->navigate("spacify:table:" + m_tableName + ":" + uuid);
	return 0;
}

int CustomTableView::onRowRightClicked(SPObject * /*sender*/,
									   SPEventArgs *args) {
	auto *rowArgs = dynamic_cast<SPTableRowEventArgs *>(args);
	if (!rowArgs || !m_dataSource)
		return 0;

	std::string uuid = m_dataSource->getRowUuidForRow(rowArgs->getRow());
	if (uuid.empty())
		return 0;

	wxWindow *win = wxTheApp ? wxTheApp->GetTopWindow() : nullptr;
	if (!win)
		return 0;

	enum { ID_EDIT = 10200, ID_DELETE };
	wxMenu menu;
	menu.Append(ID_EDIT, "Edit...");
	menu.Append(ID_DELETE, "Delete");

	int sel = -1;
	menu.Bind(wxEVT_MENU, [&sel](wxCommandEvent &ev) { sel = ev.GetId(); });
	win->PopupMenu(&menu);

	if (sel == ID_EDIT) {
		CustomRowDialog dlg(win, m_db, m_tableName, uuid);
		if (dlg.ShowModal() == wxID_OK) {
			if (m_dataSource)
				m_dataSource->reload(m_db, m_tableName);
			if (m_table)
				m_table->invalidate();
		}
	} else if (sel == ID_DELETE) {
		int confirm = wxMessageBox("Delete this row?", "Confirm",
								   wxYES_NO | wxICON_WARNING, win);
		if (confirm == wxYES) {
			std::string sql =
				"DELETE FROM user_" + m_tableName + " WHERE id=?;";
			sqlite3_stmt *stmt = nullptr;
			if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) ==
				SQLITE_OK) {
				sqlite3_bind_text(stmt, 1, uuid.c_str(), -1, SQLITE_TRANSIENT);
				sqlite3_step(stmt);
				sqlite3_finalize(stmt);
			}
			if (m_dataSource)
				m_dataSource->reload(m_db, m_tableName);
			if (m_table)
				m_table->invalidate();
		}
	}
	return 0;
}

} // namespace views
} // namespace space
