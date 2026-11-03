#include "app/library/RecordingDetailView.h"

#include "SPTabBarElement.h"
#include "Database.h"
#include "PlatformUi.h"
#include "app/library/MusicEditActions.h"

namespace space {
namespace views {

RecordingDetailView::RecordingDetailView(SPElement *parent, Database *db) :
	MusicBrowseView(parent, db) {
	addPage("appearances", "Appearances", "", {"Release", "#", "Artists", "Saved"});
	this->tabBar->addToolButton("edit", "Edit");
	this->tabBar->addToolButton("delete", "Delete");
}

bool RecordingDetailView::acceptsUri(const std::string &uri) {
	return !libraryKey("recording", uri).empty();
}

void RecordingDetailView::navigate(const std::string &uri) {
	m_isrc = libraryKey("recording", uri);
	setTabBaseUri(libraryUri("recording", m_isrc));
	MusicBrowseView::navigate(uri);
}

void RecordingDetailView::reload() {
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

void RecordingDetailView::onActivate(Page &page, int row) {
	playFrom(page, row);
}

void RecordingDetailView::onContextMenu(Page &page, int row) {
	trackContextMenu(page, row, kTrackOpenRelease);
}

void RecordingDetailView::onToolButton(const std::string &id) {
	Database::RecordingRecord rec = m_db->getRecording(m_isrc);
	if (rec.isrc.empty())
		return;
	if (id == "edit") {
		if (!edit::recording(*m_db, m_isrc).empty())
			refresh();
	} else if (id == "delete") {
		if (platform::showConfirmDialog("Confirm Delete", "Delete the recording \"" + rec.name +
					"\"? Its tracks and library saves are deleted too.")) {
			m_db->deleteRecording(m_isrc);
			open("bungalow:library:recordings");
		}
	}
}

} // namespace views
} // namespace space
