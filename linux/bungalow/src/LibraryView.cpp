#include "LibraryView.h"
#include "../../space/src/SPColor.h"
#include "../../space/src/SPPageElement.h"
#include "../../space/src/SPPlaybackEventArgs.h"
#include "../../space/src/SPTabBarElement.h"
#include "../../space/src/SPTabBarViewElement.h"
#include "../../space/src/SPTabStackElement.h"
#include "../../space/src/SPTableElement.h"
#include "AddTrackDialog.h"
#include "LibraryDataSource.h"
#include "PlayContext.h"
#include "BungalowApp.h"

#include <regex>
#include <wx/app.h>
#include <wx/menu.h>

namespace space {
namespace views {

LibraryView::LibraryView() :
	SPTabBarViewElement() {}

LibraryView::LibraryView(SPElement *parent, sqlite3 *db) :
	SPTabBarViewElement(parent) {
	auto *page = new SPPageElement(this->tabStack);
	page->setId("overview");
	page->setStringAttribute("title", "Library");
	page->setFlex(1);

	m_db = db;
	m_dataSource = new LibraryDataSource(db);
	m_table = new SPTableElement(page, m_dataSource);
	m_table->fullView = true;
	page->appendChild(m_table);
	m_table->setFlex(1);

	// Listen for double-click on any row
	m_table->addEventListener(
		"rowdblclicked",
		reinterpret_cast<SPCallback>(&LibraryView::onRowDblClicked), this,
		m_table);

	// Listen for right-click on any row
	m_table->addEventListener(
		"rowrightclicked",
		reinterpret_cast<SPCallback>(&LibraryView::onRowRightClicked), this,
		m_table);

	this->tabStack->appendChild(page);
	this->tabBar->addTab("overview", "Library", "bungalow:library");
	this->tabStack->setTabId("overview");

	// + tool button on the right of the tab bar
	this->tabBar->addToolButton("add", "+");
	this->tabBar->addEventListener(
		"toolbuttonclicked",
		reinterpret_cast<SPCallback>(&LibraryView::onToolButtonClicked), this,
		this->tabBar);
}

void LibraryView::playRowInContext(int row) {
	auto *app = dynamic_cast<BungalowApp *>(wxTheApp);
	if (!app || !m_dataSource || !m_db)
		return;

	PlayContext ctx;
	ctx.entries = m_dataSource->buildQueueEntries(m_db);
	ctx.startIndex = static_cast<std::size_t>(row);
	app->playContext(std::move(ctx));
}

int LibraryView::onRowDblClicked(SPObject * /*sender*/, SPEventArgs *args) {
	auto *rowArgs = dynamic_cast<SPTableRowEventArgs *>(args);
	if (!rowArgs)
		return 0;

	playRowInContext(rowArgs->getRow());
	return 0;
}

int LibraryView::onToolButtonClicked(SPObject * /*sender*/, SPEventArgs *args) {
	auto *btnArgs = dynamic_cast<space::TabBarToolButtonEventArgs *>(args);
	if (!btnArgs || btnArgs->buttonId != "add")
		return 0;

	wxWindow *parent = wxTheApp ? wxTheApp->GetTopWindow() : nullptr;
	if (!parent || !m_db)
		return 0;

	AddTrackDialog dlg(parent, m_db);
	if (dlg.ShowModal() == wxID_OK) {
		if (m_dataSource)
			m_dataSource->reload(m_db);
		if (m_table)
			m_table->invalidate();
	}
	return 0;
}

int LibraryView::onRowRightClicked(SPObject * /*sender*/, SPEventArgs *args) {
	auto *rowArgs = dynamic_cast<SPTableRowEventArgs *>(args);
	if (!rowArgs || !m_dataSource)
		return 0;

	const int trackId = m_dataSource->getTrackIdForRow(rowArgs->getRow());
	if (trackId <= 0)
		return 0;

	wxWindow *win = wxTheApp ? wxTheApp->GetTopWindow() : nullptr;
	if (!win)
		return 0;

	enum { ID_PLAY = 10100, ID_EDIT };
	wxMenu menu;
	menu.Append(ID_PLAY, "Play");
	menu.Append(ID_EDIT, "Edit...");

	int sel = -1;
	menu.Bind(wxEVT_MENU, [&sel](wxCommandEvent &ev) { sel = ev.GetId(); });
	win->PopupMenu(&menu);

	if (sel == ID_PLAY) {
		playRowInContext(rowArgs->getRow());
	} else if (sel == ID_EDIT) {
		AddTrackDialog dlg(win, m_db, trackId);
		if (dlg.ShowModal() == wxID_OK) {
			if (m_dataSource)
				m_dataSource->reload(m_db);
			if (m_table)
				m_table->invalidate();
		}
	}
	return 0;
}

void LibraryView::navigate(const std::string & /*uri*/) {
	if (m_dataSource && m_db) {
		m_dataSource->reload(m_db);
		if (m_table)
			m_table->invalidate();
	}
}

void LibraryView::setHash(const std::string &hash) {
	SPTabBarViewElement::setHash(hash.empty() ? "overview" : hash);
}

bool LibraryView::acceptsUri(const std::string &uri) {
	return std::regex_match(uri, std::regex("bungalow:library"));
}

LibraryView::~LibraryView() {}

} // namespace views
} // namespace space
