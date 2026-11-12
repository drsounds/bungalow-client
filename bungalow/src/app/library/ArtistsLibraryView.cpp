#include "app/library/ArtistsLibraryView.h"

#include "SPTabBarElement.h"
#include "Database.h"
#include "PlatformUi.h"
#include "app/library/MusicEditActions.h"
#include "app/library/MusicEntityRepository.h"

namespace space {
namespace views {

ArtistsLibraryView::ArtistsLibraryView(SPElement *parent, Database *db) :
	MusicBrowseView(parent, db) {
	addPage("artists", "Artists", "bungalow:library:artists",
			{MusicColumn::Name, MusicColumn::Isni, MusicColumn::RecordingCount,
			 MusicColumn::ReleaseCount});
	this->tabBar->addToolButton("add", "+");
}

bool ArtistsLibraryView::acceptsUri(const std::string &uri) {
	return uri == "bungalow:library:artists";
}

void ArtistsLibraryView::reload() {
	m_pages.front()->source->setEntities(MusicEntityRepository(*m_db).artists());
}

void ArtistsLibraryView::onActivate(Page &page, int row) {
	open(libraryUri("artist", page.source->rowAt(row)->id()));
}

void ArtistsLibraryView::onContextMenu(Page &page, int row) {
	// Holding the entity keeps it alive when refresh() replaces the rows.
	const auto artist = page.source->rowAt(row);
	enum { ID_OPEN = 10400, ID_EDIT, ID_DELETE };
	switch (platform::showPopupMenu({{ID_OPEN, "Open"},
					   {ID_EDIT, "Edit..."},
					   {0, ""},
					   {ID_DELETE, "Delete Artist"}})) {
	case ID_OPEN:
		open(libraryUri("artist", artist->id()));
		break;
	case ID_EDIT:
		if (!edit::artist(*m_db, artist->id()).empty())
			refresh();
		break;
	case ID_DELETE:
		if (platform::showConfirmDialog("Confirm Delete", "Delete the artist \"" + artist->name() +
					"\"? Their credits are removed; recordings and releases stay.")) {
			m_db->deleteArtist(artist->id());
			refresh();
		}
		break;
	default:
		break;
	}
}

void ArtistsLibraryView::onToolButton(const std::string &id) {
	if (id == "add" && !edit::artist(*m_db).empty())
		refresh();
}

} // namespace views
} // namespace space
