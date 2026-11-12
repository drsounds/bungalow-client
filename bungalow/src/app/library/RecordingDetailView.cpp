#include "app/library/RecordingDetailView.h"

#include "SPTabBarElement.h"
#include "Database.h"
#include "PlatformUi.h"
#include "app/library/MusicEditActions.h"
#include "app/library/MusicEntityRepository.h"

namespace space {
namespace views {

RecordingDetailView::RecordingDetailView(SPElement *parent, Database *db) :
	MusicBrowseView(parent, db) {
	addPage("appearances", "Appearances", "",
			{MusicColumn::Appearance, MusicColumn::Number, MusicColumn::Artists, MusicColumn::Saved});
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
	const MusicEntityRepository library(*m_db);
	const auto rec = library.recording(m_isrc);
	std::string heading = rec ? rec->name() : "Recording not found";
	if (rec && !rec->recording().version.empty())
		heading += " (" + rec->recording().version + ")";
	this->tabBar->getTabs().front()->name = heading;

	m_pages.front()->source->setEntities(library.tracksForRecording(m_isrc));
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
