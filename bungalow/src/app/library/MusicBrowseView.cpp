#include "app/library/MusicBrowseView.h"

#include "app/library/MusicEditActions.h"

#include "SPPlaybackEventArgs.h"
#include "SPSectionElement.h"
#include "SPTabBarElement.h"
#include "SPTabStackElement.h"
#include "SPTableElement.h"

#include "BungalowApp.h"
#include "Database.h"
#include "PlatformUi.h"
#include "PlayContext.h"

namespace space {
namespace views {

std::string libraryUri(const std::string& kind, const std::string& key) {
	return "bungalow:library:" + kind + ":" + key;
}

std::string libraryKey(const std::string& kind, const std::string& uri) {
	const std::string prefix = "bungalow:library:" + kind + ":";
	if (uri.rfind(prefix, 0) != 0)
		return "";
	return uri.substr(prefix.size());
}

std::string displayId(const std::string& key) {
	return key.rfind("local:", 0) == 0 ? "" : key;
}

// ── Construction ──────────────────────────────────────────────────────────────

MusicBrowseView::MusicBrowseView(SPElement* parent, Database* db) :
	SPTabBarViewElement(parent), m_db(db) {
	this->tabBar->addEventListener(
		"toolbuttonclicked",
		reinterpret_cast<SPCallback>(&MusicBrowseView::onToolButtonClicked),
		this, this->tabBar);
}

MusicBrowseView::~MusicBrowseView() {}

MusicBrowseView::Page& MusicBrowseView::addPage(const std::string& id,
												const std::string& title,
												const std::string& uri,
												std::vector<std::string> columns) {
	auto page = std::make_unique<Page>();
	page->id = id;
	page->source = std::make_unique<MusicEntityDataSource>(std::move(columns));

	auto* section = new SPSectionElement(this->tabStack);
	section->setId(id);
	section->setStringAttribute("title", title);
	section->setFlex(1);

	page->table = new SPTableElement(section, page->source.get());
	page->table->fullView = true;
	page->table->setFlex(1);
	section->appendChild(page->table);

	page->table->addEventListener(
		"rowdblclicked",
		reinterpret_cast<SPCallback>(&MusicBrowseView::onRowDblClicked), this,
		page->table);
	page->table->addEventListener(
		"rowrightclicked",
		reinterpret_cast<SPCallback>(&MusicBrowseView::onRowRightClicked), this,
		page->table);

	this->tabStack->appendChild(section);
	this->tabBar->addTab(id, title, uri);
	if (m_pages.empty())
		this->tabStack->setTabId(id);

	m_pages.push_back(std::move(page));
	return *m_pages.back();
}

void MusicBrowseView::setTabBaseUri(const std::string& uri) {
	for (SPTab* tab : this->tabBar->getTabs())
		tab->uri = uri + "#" + tab->getId();
}

// ── Navigation ────────────────────────────────────────────────────────────────

void MusicBrowseView::navigate(const std::string& /*uri*/) {
	refresh();
}

void MusicBrowseView::setHash(const std::string& hash) {
	std::string id = hash;
	if (id.empty() && !m_pages.empty())
		id = m_pages.front()->id;
	SPTabBarViewElement::setHash(id);
	// Keep the tab highlight in step when the page is chosen by URI.
	for (SPTab* tab : this->tabBar->getTabs())
		if (tab->getId() == id)
			this->tabBar->activeTab = tab;
	this->tabBar->invalidate();
}

void MusicBrowseView::refresh() {
	if (!m_db)
		return;
	reload();
	for (auto& page : m_pages)
		page->table->invalidate();
}

// ── Event dispatch ────────────────────────────────────────────────────────────

MusicBrowseView::Page* MusicBrowseView::pageFor(SPObject* sender) {
	for (auto& page : m_pages)
		if (static_cast<SPObject*>(page->table) == sender)
			return page.get();
	return nullptr;
}

int MusicBrowseView::onRowDblClicked(SPObject* sender, SPEventArgs* args) {
	auto* rowArgs = dynamic_cast<SPTableRowEventArgs*>(args);
	Page* page = pageFor(sender);
	if (!rowArgs || !page || !page->source->rowAt(rowArgs->getRow()))
		return 0;
	onActivate(*page, rowArgs->getRow());
	return 0;
}

int MusicBrowseView::onRowRightClicked(SPObject* sender, SPEventArgs* args) {
	auto* rowArgs = dynamic_cast<SPTableRowEventArgs*>(args);
	Page* page = pageFor(sender);
	if (!rowArgs || !page || !page->source->rowAt(rowArgs->getRow()))
		return 0;
	onContextMenu(*page, rowArgs->getRow());
	return 0;
}

int MusicBrowseView::onToolButtonClicked(SPObject* /*sender*/, SPEventArgs* args) {
	auto* btnArgs = dynamic_cast<space::TabBarToolButtonEventArgs*>(args);
	if (!btnArgs || !m_db)
		return 0;
	onToolButton(btnArgs->buttonId);
	return 0;
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void MusicBrowseView::playFrom(Page& page, int row) {
	auto* app = BungalowApp::current();
	const MusicEntityRow* r = page.source->rowAt(row);
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

void MusicBrowseView::trackContextMenu(Page& page, int row, unsigned flags) {
	const MusicEntityRow* r = page.source->rowAt(row);
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
	platform::MenuItems items = {{ID_PLAY, "Play"}};
	if (track.saved)
		items.push_back({ID_UNSAVE, "Remove from Library"});
	else
		items.push_back({ID_SAVE, "Save to Library"});
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

	switch (platform::showPopupMenu(items)) {
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
		open(libraryUri("release", track.releaseKey));
		break;
	case ID_OPEN_RECORDING:
		open(libraryUri("recording", track.key));
		break;
	case ID_EDIT:
		if (platform::showAddTrackDialog(m_db->handle(), track.trackId))
			refresh();
		break;
	case ID_NUMBER:
		if (edit::trackNumber(*m_db, track.trackId, track.number))
			refresh();
		break;
	case ID_DELETE:
		if (platform::showConfirmDialog("Delete Track", "Delete the track \"" + track.title +
										"\"? The recording stays in the library.")) {
			m_db->deleteTrack(track.trackId);
			refresh();
		}
		break;
	default:
		break;
	}
}

void MusicBrowseView::open(const std::string& uri) {
	if (auto* app = BungalowApp::current())
		app->navigate(uri);
}

} // namespace views
} // namespace space
