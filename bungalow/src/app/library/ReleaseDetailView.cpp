#include "app/library/ReleaseDetailView.h"

#include "SPTabBarElement.h"
#include "Database.h"
#include "PlatformUi.h"
#include "app/library/MusicEditActions.h"
#include "app/library/MusicEntityRepository.h"

namespace space {
namespace views {

ReleaseDetailView::ReleaseDetailView(SPElement *parent, Database *db) :
	MusicBrowseView(parent, db) {
	addPage("tracks", "Tracks", "",
			{MusicColumn::Number, MusicColumn::Name, MusicColumn::Version, MusicColumn::Artists,
			 MusicColumn::Saved});
	this->tabBar->addToolButton("add", "+");
	this->tabBar->addToolButton("edit", "Edit");
	this->tabBar->addToolButton("delete", "Delete");
}

bool ReleaseDetailView::acceptsUri(const std::string &uri) {
	return !libraryKey("release", uri).empty();
}

void ReleaseDetailView::navigate(const std::string &uri) {
	m_upc = libraryKey("release", uri);
	setTabBaseUri(libraryUri("release", m_upc));
	MusicBrowseView::navigate(uri);
}

void ReleaseDetailView::reload() {
	const MusicEntityRepository library(*m_db);
	const auto release = library.release(m_upc);
	std::string heading = release ? release->name() : "Release not found";
	if (release && !release->release().releaseDate.empty())
		heading += " (" + release->release().releaseDate.substr(0, 4) + ")";
	this->tabBar->getTabs().front()->name = heading;

	m_pages.front()->source->setEntities(library.tracksForRelease(m_upc));
}

void ReleaseDetailView::onActivate(Page &page, int row) {
	playFrom(page, row);
}

void ReleaseDetailView::onContextMenu(Page &page, int row) {
	trackContextMenu(page, row, kTrackOpenRecording);
}

void ReleaseDetailView::onToolButton(const std::string &id) {
	if (m_db->getRelease(m_upc).upc.empty())
		return;
	if (id == "add") {
		if (edit::addTrackToRelease(*m_db, m_upc))
			refresh();
	} else if (id == "edit") {
		if (!edit::release(*m_db, m_upc).empty())
			refresh();
	} else if (id == "delete") {
		if (platform::showConfirmDialog("Confirm Delete", "Delete this release? Its recordings stay in the library "
					"without a release.")) {
			m_db->deleteRelease(m_upc);
			open("bungalow:library:releases");
		}
	}
}

} // namespace views
} // namespace space
