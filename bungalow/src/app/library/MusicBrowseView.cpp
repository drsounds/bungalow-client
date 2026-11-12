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

// ── Construction ──────────────────────────────────────────────────────────────

MusicBrowseView::MusicBrowseView(SPElement* parent, Database* db) :
	SPTabBarViewElement(parent), m_db(db) {
	this->tabBar->addEventListener(
		"toolbuttonclicked",
		reinterpret_cast<SPCallback>(&MusicBrowseView::onToolButtonClicked),
		this, this->tabBar);
	if (auto* app = BungalowApp::current()) {
		if (auto resolver = app->getAvailabilityResolver().lock()) {
			m_availabilityChanged = resolver->subscribe([this] {
				for (auto& page : m_pages)
					page->table->invalidate();
			});
		}
	}
}

MusicBrowseView::~MusicBrowseView() {}

MusicBrowseView::Page& MusicBrowseView::addPage(const std::string& id,
												const std::string& title,
												const std::string& uri,
												std::vector<MusicColumn> columns) {
	auto page = std::make_unique<Page>();
	page->id = id;
	page->source = std::make_unique<MusicEntityDataSource>(std::move(columns));
	if (auto* app = BungalowApp::current())
		page->source->setAvailability(app->getAvailabilityResolver());

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
	page->table->setDropHandler(
		[this](const SPDragData&) { return acceptsDrop(); },
		[this](const SPDragData& data, int index) { return onDropRefs(data.uris, index); });

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
	// A URI without a hash arrives as "overview" (SPUri's default), which is not
	// one of our page ids; anything unknown falls back to the first page.
	std::string id = hash;
	bool known = false;
	for (const auto& page : m_pages)
		if (page->id == id)
			known = true;
	if (!known && !m_pages.empty())
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
	if (auto* app = BungalowApp::current())
		if (auto resolver = app->getAvailabilityResolver().lock())
			resolver->clear();
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

void MusicBrowseView::playFrom(Page& page, int row, bool chooseService) {
	auto* app = BungalowApp::current();
	const auto entity = page.source->rowAt(row);
	if (!app || !entity)
		return;
	const bool resolvable = chooseService && !entity->isrc().empty();
	if (!page.source->isRowPlayable(row) && !resolvable) {
		app->showError("\"" + entity->name() + "\" has no playable file or stream.");
		return;
	}
	PlayContext ctx;
	ctx.entries = page.source->buildQueueEntries();
	ctx.startIndex = static_cast<std::size_t>(row);
	ctx.chooseService = chooseService;
	if (resolvable && ctx.startIndex < ctx.entries.size())
		ctx.entries[ctx.startIndex].playable = true;
	app->playContext(std::move(ctx));
}

void MusicBrowseView::trackContextMenu(Page& page, int row, unsigned flags) {
	// Holding the entity keeps it alive when refresh() replaces the rows.
	const auto track = page.source->rowAs<TrackEntity>(row);
	if (!track || !m_db)
		return;
	const int trackId = static_cast<int>(track->trackId());
	const std::string releaseKey = track->release() ? track->release()->upc : "";

	enum {
		ID_PLAY = 10300,
		ID_PLAY_WITH,
		ID_SAVE,
		ID_UNSAVE,
		ID_OPEN_RELEASE,
		ID_OPEN_RECORDING,
		ID_EDIT,
		ID_NUMBER,
		ID_DELETE
	};
	platform::MenuItems items = {{ID_PLAY, "Play"}};
	if (!track->isrc().empty())
		items.push_back({ID_PLAY_WITH, "Play with..."});
	if (track->saved())
		items.push_back({ID_UNSAVE, "Remove from Library"});
	else
		items.push_back({ID_SAVE, "Save to Library"});
	items.push_back({0, ""});
	if ((flags & kTrackOpenRelease) && !releaseKey.empty())
		items.push_back({ID_OPEN_RELEASE, "Go to Release"});
	if (flags & kTrackOpenRecording)
		items.push_back({ID_OPEN_RECORDING, "Go to Recording"});
	items.push_back({ID_EDIT, "Edit..."});
	if (!releaseKey.empty())
		items.push_back({ID_NUMBER, "Track Number..."});
	items.push_back({0, ""});
	items.push_back({ID_DELETE, "Delete Track"});

	switch (platform::showPopupMenu(items)) {
	case ID_PLAY:
		playFrom(page, row);
		break;
	case ID_PLAY_WITH:
		playFrom(page, row, true);
		break;
	case ID_SAVE:
		m_db->ensureSave(trackId);
		refresh();
		break;
	case ID_UNSAVE:
		m_db->removeSave(trackId);
		refresh();
		break;
	case ID_OPEN_RELEASE:
		open(libraryUri("release", releaseKey));
		break;
	case ID_OPEN_RECORDING:
		open(libraryUri("recording", track->recording().isrc));
		break;
	case ID_EDIT:
		if (platform::showAddTrackDialog(m_db->handle(), trackId))
			refresh();
		break;
	case ID_NUMBER:
		if (edit::trackNumber(*m_db, trackId, track->track().number))
			refresh();
		break;
	case ID_DELETE:
		if (platform::showConfirmDialog("Delete Track", "Delete the track \"" + track->name() +
										"\"? The recording stays in the library.")) {
			m_db->deleteTrack(trackId);
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
