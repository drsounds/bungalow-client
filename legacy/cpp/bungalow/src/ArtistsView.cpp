#include "ArtistsView.h"

#include "../../space/src/SPTabBarElement.h"
#include "Database.h"
#include "MusicEditActions.h"

namespace space {
namespace views {

ArtistsView::ArtistsView(SPElement *parent, Database *db) :
	MusicBrowseView(parent, db) {
	addPage("artists", "Artists", "bungalow:library:artists",
			{"Name", "ISNI", "Recordings", "Releases"});
	this->tabBar->addToolButton("add", "+");
}

bool ArtistsView::acceptsUri(const std::string &uri) {
	return uri == "bungalow:library:artists";
}

void ArtistsView::reload() {
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

void ArtistsView::onActivate(Page &page, int row) {
	open(entityUri("artist", page.source->rowAt(row)->key));
}

void ArtistsView::onContextMenu(Page &page, int row) {
	const MusicEntityRow artist = *page.source->rowAt(row);
	enum { ID_OPEN = 10400, ID_EDIT, ID_DELETE };
	switch (popupMenu({{ID_OPEN, "Open"},
					   {ID_EDIT, "Edit..."},
					   {0, ""},
					   {ID_DELETE, "Delete Artist"}})) {
	case ID_OPEN:
		open(entityUri("artist", artist.key));
		break;
	case ID_EDIT:
		if (!edit::artist(topWindow(), *m_db, artist.key).empty())
			refresh();
		break;
	case ID_DELETE:
		if (confirm("Delete the artist \"" + artist.title +
					"\"? Their credits are removed; recordings and releases stay.")) {
			m_db->deleteArtist(artist.key);
			refresh();
		}
		break;
	default:
		break;
	}
}

void ArtistsView::onToolButton(const std::string &id) {
	if (id == "add" && !edit::artist(topWindow(), *m_db).empty())
		refresh();
}

} // namespace views
} // namespace space
