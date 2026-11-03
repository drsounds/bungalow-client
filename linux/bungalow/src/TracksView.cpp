#include "TracksView.h"

#include "../../space/src/SPTabBarElement.h"
#include "AddTrackDialog.h"
#include "Database.h"

namespace space {
namespace views {

TracksView::TracksView(SPElement *parent, Database *db) :
	MusicBrowseView(parent, db) {
	addPage("tracks", "All Tracks", "bungalow:library:tracks",
			{"Name", "Artists", "Release", "#", "Saved"});
	this->tabBar->addToolButton("add", "+");
}

bool TracksView::acceptsUri(const std::string &uri) {
	return uri == "bungalow:library:tracks";
}

void TracksView::reload() {
	m_pages.front()->source->setTrackRows(
		m_db->getAllTracks(), [](const Database::TrackRow &t) {
			return std::vector<std::string>{
				t.name, t.artists, t.releaseName,
				t.number > 0 ? std::to_string(t.number) : "",
				t.saved ? "Yes" : ""};
		});
}

void TracksView::onActivate(Page &page, int row) {
	playFrom(page, row);
}

void TracksView::onContextMenu(Page &page, int row) {
	trackContextMenu(page, row, kTrackOpenRelease | kTrackOpenRecording);
}

void TracksView::onToolButton(const std::string &id) {
	if (id != "add")
		return;
	AddTrackDialog dlg(topWindow(), m_db->handle());
	if (dlg.ShowModal() == wxID_OK)
		refresh();
}

} // namespace views
} // namespace space
