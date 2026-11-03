#include "app/library/TracksLibraryView.h"

#include "SPTabBarElement.h"
#include "PlatformUi.h"
#include "Database.h"

namespace space {
namespace views {

TracksLibraryView::TracksLibraryView(SPElement *parent, Database *db) :
	MusicBrowseView(parent, db) {
	addPage("tracks", "All Tracks", "bungalow:library:tracks",
			{"Name", "Artists", "Release", "#", "Saved"});
	this->tabBar->addToolButton("add", "+");
}

bool TracksLibraryView::acceptsUri(const std::string &uri) {
	return uri == "bungalow:library:tracks";
}

void TracksLibraryView::reload() {
	m_pages.front()->source->setTrackRows(
		m_db->getAllTracks(), [](const Database::TrackRow &t) {
			return std::vector<std::string>{
				t.name, t.artists, t.releaseName,
				t.number > 0 ? std::to_string(t.number) : "",
				t.saved ? "Yes" : ""};
		});
}

void TracksLibraryView::onActivate(Page &page, int row) {
	playFrom(page, row);
}

void TracksLibraryView::onContextMenu(Page &page, int row) {
	trackContextMenu(page, row, kTrackOpenRelease | kTrackOpenRecording);
}

void TracksLibraryView::onToolButton(const std::string &id) {
	if (id != "add")
		return;
	if (platform::showAddTrackDialog(m_db->handle()))
		refresh();
}

} // namespace views
} // namespace space
