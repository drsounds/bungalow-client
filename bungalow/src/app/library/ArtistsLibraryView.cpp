#include "app/library/ArtistsLibraryView.h"

#include "SPTabBarElement.h"
#include "Database.h"
#include "PlatformUi.h"
#include "app/library/MusicEditActions.h"

namespace space {
namespace views {

ArtistsLibraryView::ArtistsLibraryView(SPElement *parent, Database *db) :
	MusicBrowseView(parent, db) {
	addPage("artists", "Artists", "bungalow:library:artists",
			{"Name", "ISNI", "Recordings", "Releases"});
	this->tabBar->addToolButton("add", "+");
}

bool ArtistsLibraryView::acceptsUri(const std::string &uri) {
	return uri == "bungalow:library:artists";
}

void ArtistsLibraryView::reload() {
	std::vector<MusicEntityRow> rows;
	for (const auto &a : m_db->getArtistSummaries()) {
		MusicEntityRow r;
		r.key = a.artist.isni;
		r.title = a.artist.name;
		r.playable = true; // not greyed out: rows open the artist page
		r.cells = {a.artist.name, displayId(a.artist.isni),
				   std::to_string(a.recordingCount),
				   std::to_string(a.releaseCount)};
		rows.push_back(std::move(r));
	}
	m_pages.front()->source->setRows(std::move(rows));
}

void ArtistsLibraryView::onActivate(Page &page, int row) {
	open(libraryUri("artist", page.source->rowAt(row)->key));
}

void ArtistsLibraryView::onContextMenu(Page &page, int row) {
	const MusicEntityRow artist = *page.source->rowAt(row);
	enum { ID_OPEN = 10400, ID_EDIT, ID_DELETE };
	switch (platform::showPopupMenu({{ID_OPEN, "Open"},
					   {ID_EDIT, "Edit..."},
					   {0, ""},
					   {ID_DELETE, "Delete Artist"}})) {
	case ID_OPEN:
		open(libraryUri("artist", artist.key));
		break;
	case ID_EDIT:
		if (!edit::artist(*m_db, artist.key).empty())
			refresh();
		break;
	case ID_DELETE:
		if (platform::showConfirmDialog("Confirm Delete", "Delete the artist \"" + artist.title +
					"\"? Their credits are removed; recordings and releases stay.")) {
			m_db->deleteArtist(artist.key);
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
