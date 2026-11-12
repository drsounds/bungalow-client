#include "app/library/ArtistDetailView.h"

#include "SPTabBarElement.h"
#include "Database.h"
#include "PlatformUi.h"
#include "app/library/MusicEditActions.h"
#include "app/library/MusicEntityRepository.h"

namespace space {
namespace views {

ArtistDetailView::ArtistDetailView(SPElement *parent, Database *db) :
	MusicBrowseView(parent, db) {
	addPage("tracks", "Tracks", "",
			{MusicColumn::Name, MusicColumn::Release, MusicColumn::Artists, MusicColumn::Saved});
	addPage("releases", "Releases", "",
			{MusicColumn::Name, MusicColumn::Date, MusicColumn::Artists, MusicColumn::TrackCount});
	this->tabBar->addToolButton("edit", "Edit");
	this->tabBar->addToolButton("delete", "Delete");
}

bool ArtistDetailView::acceptsUri(const std::string &uri) {
	return !libraryKey("artist", uri).empty();
}

void ArtistDetailView::navigate(const std::string &uri) {
	m_isni = libraryKey("artist", uri);
	setTabBaseUri(libraryUri("artist", m_isni));
	MusicBrowseView::navigate(uri);
}

void ArtistDetailView::reload() {
	const MusicEntityRepository library(*m_db);
	const auto artist = library.artist(m_isni);
	// The first tab carries the artist's name, as the page heading.
	this->tabBar->getTabs().front()->name = artist ? artist->name() : "Artist not found";

	m_pages[0]->source->setEntities(library.tracksForArtist(m_isni));
	m_pages[1]->source->setEntities(library.releasesForArtist(m_isni));
}

void ArtistDetailView::onActivate(Page &page, int row) {
	if (page.id == "releases")
		open(libraryUri("release", page.source->rowAt(row)->id()));
	else
		playFrom(page, row);
}

void ArtistDetailView::onContextMenu(Page &page, int row) {
	if (page.id == "tracks") {
		trackContextMenu(page, row, kTrackOpenRelease | kTrackOpenRecording);
		return;
	}
	enum { ID_OPEN = 10700 };
	if (platform::showPopupMenu({{ID_OPEN, "Go to Release"}}) == ID_OPEN)
		open(libraryUri("release", page.source->rowAt(row)->id()));
}

void ArtistDetailView::onToolButton(const std::string &id) {
	if (id == "edit") {
		if (!edit::artist(*m_db, m_isni).empty())
			refresh();
	} else if (id == "delete") {
		Database::ArtistRecord artist = m_db->getArtist(m_isni);
		if (!artist.isni.empty() &&
			platform::showConfirmDialog("Confirm Delete", "Delete the artist \"" + artist.name +
					"\"? Their credits are removed; recordings and releases stay.")) {
			m_db->deleteArtist(m_isni);
			open("bungalow:library:artists");
		}
	}
}

} // namespace views
} // namespace space
