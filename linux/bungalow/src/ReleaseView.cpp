#include "ReleaseView.h"

#include "../../space/src/SPTabBarElement.h"
#include "Database.h"
#include "MusicEditActions.h"

namespace space {
namespace views {

ReleaseView::ReleaseView(SPElement *parent, Database *db) :
	MusicBrowseView(parent, db) {
	addPage("tracks", "Tracks", "", {"#", "Name", "Version", "Artists", "Saved"});
	this->tabBar->addToolButton("add", "+");
	this->tabBar->addToolButton("edit", "Edit");
	this->tabBar->addToolButton("delete", "Delete");
}

bool ReleaseView::acceptsUri(const std::string &uri) {
	return !entityKey("release", uri).empty();
}

void ReleaseView::navigate(const std::string &uri) {
	m_upc = entityKey("release", uri);
	setTabBaseUri(entityUri("release", m_upc));
	MusicBrowseView::navigate(uri);
}

void ReleaseView::reload() {
	Database::ReleaseRecord release = m_db->getRelease(m_upc);
	std::string heading = release.upc.empty() ? "Release not found" : release.name;
	if (!release.releaseDate.empty())
		heading += " (" + release.releaseDate.substr(0, 4) + ")";
	this->tabBar->getTabs().front()->name = heading;

	m_pages.front()->source->setTrackRows(
		m_db->getTracksForRelease(m_upc), [](const Database::TrackRow &t) {
			return std::vector<std::string>{
				t.number > 0 ? std::to_string(t.number) : "", t.name,
				t.version, t.artists, t.saved ? "Yes" : ""};
		});
}

void ReleaseView::onActivate(Page &page, int row) {
	playFrom(page, row);
}

void ReleaseView::onContextMenu(Page &page, int row) {
	trackContextMenu(page, row, kTrackOpenRecording);
}

void ReleaseView::onToolButton(const std::string &id) {
	if (m_db->getRelease(m_upc).upc.empty())
		return;
	if (id == "add") {
		if (edit::addTrackToRelease(topWindow(), *m_db, m_upc))
			refresh();
	} else if (id == "edit") {
		if (!edit::release(topWindow(), *m_db, m_upc).empty())
			refresh();
	} else if (id == "delete") {
		if (confirm("Delete this release? Its recordings stay in the library "
					"without a release.")) {
			m_db->deleteRelease(m_upc);
			open("bungalow:library:releases");
		}
	}
}

} // namespace views
} // namespace space
