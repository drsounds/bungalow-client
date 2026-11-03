#include "ReleasesView.h"

#include "../../space/src/SPTabBarElement.h"
#include "Database.h"
#include "MusicEditActions.h"

namespace space {
namespace views {

ReleasesView::ReleasesView(SPElement *parent, Database *db) :
	MusicBrowseView(parent, db) {
	addPage("releases", "Releases", "bungalow:library:releases",
			{"Name", "Artists", "Date", "Tracks", "UPC"});
	this->tabBar->addToolButton("add", "+");
}

bool ReleasesView::acceptsUri(const std::string &uri) {
	return uri == "bungalow:library:releases";
}

void ReleasesView::reload() {
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

void ReleasesView::onActivate(Page &page, int row) {
	open(entityUri("release", page.source->rowAt(row)->key));
}

void ReleasesView::onContextMenu(Page &page, int row) {
	const MusicEntityRow rel = *page.source->rowAt(row);
	enum { ID_OPEN = 10600, ID_EDIT, ID_DELETE };
	switch (popupMenu({{ID_OPEN, "Open"},
					   {ID_EDIT, "Edit..."},
					   {0, ""},
					   {ID_DELETE, "Delete Release"}})) {
	case ID_OPEN:
		open(entityUri("release", rel.key));
		break;
	case ID_EDIT:
		if (!edit::release(topWindow(), *m_db, rel.key).empty())
			refresh();
		break;
	case ID_DELETE:
		if (confirm("Delete the release \"" + rel.title +
					"\"? Its recordings stay in the library without a release.")) {
			m_db->deleteRelease(rel.key);
			refresh();
		}
		break;
	default:
		break;
	}
}

void ReleasesView::onToolButton(const std::string &id) {
	if (id != "add")
		return;
	std::string upc = edit::release(topWindow(), *m_db);
	if (!upc.empty())
		open(entityUri("release", upc)); // straight to the page where tracks are added
}

} // namespace views
} // namespace space
