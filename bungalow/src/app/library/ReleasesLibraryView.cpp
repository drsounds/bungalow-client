#include "app/library/ReleasesLibraryView.h"

#include "SPTabBarElement.h"
#include "Database.h"
#include "PlatformUi.h"
#include "app/library/MusicEditActions.h"

namespace space {
namespace views {

ReleasesLibraryView::ReleasesLibraryView(SPElement *parent, Database *db) :
	MusicBrowseView(parent, db) {
	addPage("releases", "Releases", "bungalow:library:releases",
			{"Name", "Artists", "Date", "Tracks", "UPC"});
	this->tabBar->addToolButton("add", "+");
}

bool ReleasesLibraryView::acceptsUri(const std::string &uri) {
	return uri == "bungalow:library:releases";
}

void ReleasesLibraryView::reload() {
	std::vector<MusicEntityRow> rows;
	for (const auto &rel : m_db->getReleaseSummaries()) {
		MusicEntityRow r;
		r.key = rel.release.upc;
		r.title = rel.release.name;
		r.artist = rel.artists;
		r.cells = {rel.release.name, rel.artists, rel.release.releaseDate,
				   std::to_string(rel.trackCount), displayId(rel.release.upc)};
		rows.push_back(std::move(r));
	}
	m_pages.front()->source->setRows(std::move(rows));
}

void ReleasesLibraryView::onActivate(Page &page, int row) {
	open(libraryUri("release", page.source->rowAt(row)->key));
}

void ReleasesLibraryView::onContextMenu(Page &page, int row) {
	const MusicEntityRow rel = *page.source->rowAt(row);
	enum { ID_OPEN = 10600, ID_EDIT, ID_DELETE };
	switch (platform::showPopupMenu({{ID_OPEN, "Open"},
					   {ID_EDIT, "Edit..."},
					   {0, ""},
					   {ID_DELETE, "Delete Release"}})) {
	case ID_OPEN:
		open(libraryUri("release", rel.key));
		break;
	case ID_EDIT:
		if (!edit::release(*m_db, rel.key).empty())
			refresh();
		break;
	case ID_DELETE:
		if (platform::showConfirmDialog("Confirm Delete", "Delete the release \"" + rel.title +
					"\"? Its recordings stay in the library without a release.")) {
			m_db->deleteRelease(rel.key);
			refresh();
		}
		break;
	default:
		break;
	}
}

void ReleasesLibraryView::onToolButton(const std::string &id) {
	if (id != "add")
		return;
	std::string upc = edit::release(*m_db);
	if (!upc.empty())
		open(libraryUri("release", upc)); // straight to the page where tracks are added
}

} // namespace views
} // namespace space
