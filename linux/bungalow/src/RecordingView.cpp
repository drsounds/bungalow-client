#include "RecordingView.h"

#include "../../space/src/SPTabBarElement.h"
#include "Database.h"
#include "MusicEditActions.h"

namespace space {
namespace views {

RecordingView::RecordingView(SPElement *parent, Database *db) :
	MusicBrowseView(parent, db) {
	addPage("appearances", "Appearances", "", {"Release", "#", "Artists", "Saved"});
	this->tabBar->addToolButton("edit", "Edit");
	this->tabBar->addToolButton("delete", "Delete");
}

bool RecordingView::acceptsUri(const std::string &uri) {
	return !entityKey("recording", uri).empty();
}

void RecordingView::navigate(const std::string &uri) {
	m_isrc = entityKey("recording", uri);
	setTabBaseUri(entityUri("recording", m_isrc));
	MusicBrowseView::navigate(uri);
}

void RecordingView::reload() {
	Database::RecordingRecord rec = m_db->getRecording(m_isrc);
	std::string heading = rec.isrc.empty() ? "Recording not found" : rec.name;
	if (!rec.version.empty())
		heading += " (" + rec.version + ")";
	this->tabBar->getTabs().front()->name = heading;

	m_pages.front()->source->setTrackRows(
		m_db->getTracksForRecording(m_isrc), [](const Database::TrackRow &t) {
			return std::vector<std::string>{
				t.releaseName.empty() ? "(no release)" : t.releaseName,
				t.number > 0 ? std::to_string(t.number) : "", t.artists,
				t.saved ? "Yes" : ""};
		});
}

void RecordingView::onActivate(Page &page, int row) {
	playFrom(page, row);
}

void RecordingView::onContextMenu(Page &page, int row) {
	trackContextMenu(page, row, kTrackOpenRelease);
}

void RecordingView::onToolButton(const std::string &id) {
	Database::RecordingRecord rec = m_db->getRecording(m_isrc);
	if (rec.isrc.empty())
		return;
	if (id == "edit") {
		if (!edit::recording(topWindow(), *m_db, m_isrc).empty())
			refresh();
	} else if (id == "delete") {
		if (confirm("Delete the recording \"" + rec.name +
					"\"? Its tracks and library saves are deleted too.")) {
			m_db->deleteRecording(m_isrc);
			open("bungalow:library:recordings");
		}
	}
}

} // namespace views
} // namespace space
