#include "MusicBrowseView.h"

#include "../../space/src/SPPageElement.h"
#include "../../space/src/SPPlaybackEventArgs.h"
#include "../../space/src/SPTabBarElement.h"
#include "../../space/src/SPTabStackElement.h"
#include "../../space/src/SPTableElement.h"
#include "AddTrackDialog.h"
#include "BungalowApp.h"
#include "Database.h"
#include "MusicEditActions.h"
#include "PlayContext.h"

#include <wx/app.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>

namespace space {
namespace views {

std::string entityUri(const std::string& kind, const std::string& key) {
	return "bungalow:" + kind + ":" + key;
}

std::string entityKey(const std::string& kind, const std::string& uri) {
	const std::string prefix = "bungalow:" + kind + ":";
	if (uri.rfind(prefix, 0) != 0)
		return "";
	return uri.substr(prefix.size());
}

std::string displayId(const std::string &key) {
	return key.rfind("local:", 0) == 0 ? "" : key;
}

// ── Construction ──────────────────────────────────────────────────────────────

MusicBrowseView::MusicBrowseView(SPElement *parent, Database *db) :
	SPTabBarViewElement(parent), m_db(db) {
	this->tabBar->addEventListener(
		"toolbuttonclicked",
		reinterpret_cast<SPCallback>(&MusicBrowseView::onToolButtonClicked),
		this, this->tabBar);
}

MusicBrowseView::~MusicBrowseView() {}

MusicBrowseView::Page &MusicBrowseView::addPage(const std::string &id,
												const std::string &title,
												const std::string &uri,
												std::vector<std::string> columns) {
	auto page = std::make_unique<Page>();
	page->id = id;
	page->source = std::make_unique<MusicEntityDataSource>(std::move(columns));

	auto *element = new SPPageElement(this->tabStack);
	element->setId(id);
	element->setStringAttribute("title", title);
	element->setFlex(1);

	page->table = new SPTableElement(element, page->source.get());
	page->table->fullView = true;
	page->table->setFlex(1);
	element->appendChild(page->table);

	page->table->addEventListener(
		"rowdblclicked",
		reinterpret_cast<SPCallback>(&MusicBrowseView::onRowDblClicked), this,
		page->table);
	page->table->addEventListener(
		"rowrightclicked",
		reinterpret_cast<SPCallback>(&MusicBrowseView::onRowRightClicked), this,
		page->table);

	this->tabStack->appendChild(element);
	this->tabBar->addTab(id, title, uri);
	if (m_pages.empty())
		this->tabStack->setTabId(id);

	m_pages.push_back(std::move(page));
	return *m_pages.back();
}

void MusicBrowseView::setTabBaseUri(const std::string &uri) {
	for (SPTab *tab : this->tabBar->getTabs())
		tab->uri = uri + "#" + tab->getId();
}

// ── Navigation ────────────────────────────────────────────────────────────────

void MusicBrowseView::navigate(const std::string & /*uri*/) {
	refresh();
}

void MusicBrowseView::setHash(const std::string &hash) {
	std::string id = hash;
	if (id.empty() && !m_pages.empty())
		id = m_pages.front()->id;
	SPTabBarViewElement::setHash(id);
	// Keep the tab highlight in step when the page is chosen by URI.
	for (SPTab *tab : this->tabBar->getTabs())
		if (tab->getId() == id)
			this->tabBar->activeTab = tab;
	this->tabBar->invalidate();
}

void MusicBrowseView::refresh() {
	if (!m_db)
		return;
	reload();
	for (auto &page : m_pages)
		page->table->invalidate();
}

// ── Event dispatch ────────────────────────────────────────────────────────────

MusicBrowseView::Page *MusicBrowseView::pageFor(SPObject *sender) {
	for (auto &page : m_pages)
		if (static_cast<SPObject *>(page->table) == sender)
			return page.get();
	return nullptr;
}

int MusicBrowseView::onRowDblClicked(SPObject *sender, SPEventArgs *args) {
	auto *rowArgs = dynamic_cast<SPTableRowEventArgs *>(args);
	Page *page = pageFor(sender);
	if (!rowArgs || !page || !page->source->rowAt(rowArgs->getRow()))
		return 0;
	onActivate(*page, rowArgs->getRow());
	return 0;
}

int MusicBrowseView::onRowRightClicked(SPObject *sender, SPEventArgs *args) {
	auto *rowArgs = dynamic_cast<SPTableRowEventArgs *>(args);
	Page *page = pageFor(sender);
	if (!rowArgs || !page || !page->source->rowAt(rowArgs->getRow()))
		return 0;
	onContextMenu(*page, rowArgs->getRow());
	return 0;
}

int MusicBrowseView::onToolButtonClicked(SPObject * /*sender*/, SPEventArgs *args) {
	auto *btnArgs = dynamic_cast<space::TabBarToolButtonEventArgs *>(args);
	if (!btnArgs || !m_db)
		return 0;
	onToolButton(btnArgs->buttonId);
	return 0;
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void MusicBrowseView::playFrom(Page &page, int row) {
	auto *app = dynamic_cast<BungalowApp *>(wxTheApp);
	const MusicEntityRow *r = page.source->rowAt(row);
	if (!app || !r)
		return;
	if (!r->playable) {
		app->showError("\"" + r->title + "\" has no playable file or stream.");
		return;
	}
	PlayContext ctx;
	ctx.entries = page.source->buildQueueEntries();
	ctx.startIndex = static_cast<std::size_t>(row);
	app->playContext(std::move(ctx));
}

void MusicBrowseView::trackContextMenu(Page &page, int row, unsigned flags) {
	const MusicEntityRow *r = page.source->rowAt(row);
	if (!r || !m_db)
		return;
	// Copy what the actions need: refresh() replaces the rows.
	const MusicEntityRow track = *r;

	enum {
		ID_PLAY = 10300,
		ID_SAVE,
		ID_UNSAVE,
		ID_OPEN_RELEASE,
		ID_OPEN_RECORDING,
		ID_EDIT,
		ID_NUMBER,
		ID_DELETE
	};
	std::vector<std::pair<int, std::string>> items = {{ID_PLAY, "Play"}};
	items.push_back(track.saved ? std::make_pair(int(ID_UNSAVE), std::string("Remove from Library"))
								: std::make_pair(int(ID_SAVE), std::string("Save to Library")));
	items.push_back({0, ""});
	if ((flags & kTrackOpenRelease) && !track.releaseKey.empty())
		items.push_back({ID_OPEN_RELEASE, "Go to Release"});
	if (flags & kTrackOpenRecording)
		items.push_back({ID_OPEN_RECORDING, "Go to Recording"});
	items.push_back({ID_EDIT, "Edit..."});
	if (!track.releaseKey.empty())
		items.push_back({ID_NUMBER, "Track Number..."});
	items.push_back({0, ""});
	items.push_back({ID_DELETE, "Delete Track"});

	switch (popupMenu(items)) {
	case ID_PLAY:
		playFrom(page, row);
		break;
	case ID_SAVE:
		m_db->ensureSave(track.trackId);
		refresh();
		break;
	case ID_UNSAVE:
		m_db->removeSave(track.trackId);
		refresh();
		break;
	case ID_OPEN_RELEASE:
		open(entityUri("release", track.releaseKey));
		break;
	case ID_OPEN_RECORDING:
		open(entityUri("recording", track.key));
		break;
	case ID_EDIT: {
		AddTrackDialog dlg(topWindow(), m_db->handle(), track.trackId);
		if (dlg.ShowModal() == wxID_OK)
			refresh();
		break;
	}
	case ID_NUMBER:
		if (edit::trackNumber(topWindow(), *m_db, track.trackId, track.number))
			refresh();
		break;
	case ID_DELETE:
		if (confirm("Delete the track \"" + track.title +
					"\"? The recording stays in the library.")) {
			m_db->deleteTrack(track.trackId);
			refresh();
		}
		break;
	default:
		break;
	}
}

void MusicBrowseView::open(const std::string &uri) {
	if (auto *app = dynamic_cast<BungalowApp *>(wxTheApp))
		app->navigate(uri);
}

int MusicBrowseView::popupMenu(const std::vector<std::pair<int, std::string>> &items) {
	wxWindow *win = topWindow();
	if (!win)
		return -1;
	wxMenu menu;
	for (const auto &item : items) {
		if (item.second.empty())
			menu.AppendSeparator();
		else
			menu.Append(item.first, wxString::FromUTF8(item.second));
	}
	int sel = -1;
	menu.Bind(wxEVT_MENU, [&sel](wxCommandEvent &ev) { sel = ev.GetId(); });
	win->PopupMenu(&menu);
	return sel;
}

bool MusicBrowseView::confirm(const std::string &question) {
	return wxMessageBox(wxString::FromUTF8(question), "Confirm",
						wxYES_NO | wxICON_WARNING, topWindow()) == wxYES;
}

wxWindow *MusicBrowseView::topWindow() const {
	return wxTheApp ? wxTheApp->GetTopWindow() : nullptr;
}

} // namespace views
} // namespace space
