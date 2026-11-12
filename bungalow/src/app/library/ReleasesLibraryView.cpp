#include "app/library/ReleasesLibraryView.h"

#include "SPTabBarElement.h"
#include "Database.h"
#include "PlatformUi.h"
#include "app/library/MusicEditActions.h"
#include "app/library/MusicEntityRepository.h"

namespace space {
namespace views {

ReleasesLibraryView::ReleasesLibraryView(SPElement *parent, Database *db) :
	MusicBrowseView(parent, db) {
	addPage("releases", "Releases", "bungalow:library:releases",
			{MusicColumn::Name, MusicColumn::Artists, MusicColumn::Date, MusicColumn::TrackCount,
			 MusicColumn::Upc});
	this->tabBar->addToolButton("add", "+");
}

bool ReleasesLibraryView::acceptsUri(const std::string &uri) {
	return uri == "bungalow:library:releases";
}

void ReleasesLibraryView::reload() {
	m_pages.front()->source->setEntities(MusicEntityRepository(*m_db).releases());
}

void ReleasesLibraryView::onActivate(Page &page, int row) {
	open(libraryUri("release", page.source->rowAt(row)->id()));
}

void ReleasesLibraryView::onContextMenu(Page &page, int row) {
	// Holding the entity keeps it alive when refresh() replaces the rows.
	const auto rel = page.source->rowAt(row);
	enum { ID_OPEN = 10600, ID_EDIT, ID_DELETE };
	switch (platform::showPopupMenu({{ID_OPEN, "Open"},
					   {ID_EDIT, "Edit..."},
					   {0, ""},
					   {ID_DELETE, "Delete Release"}})) {
	case ID_OPEN:
		open(libraryUri("release", rel->id()));
		break;
	case ID_EDIT:
		if (!edit::release(*m_db, rel->id()).empty())
			refresh();
		break;
	case ID_DELETE:
		if (platform::showConfirmDialog("Confirm Delete", "Delete the release \"" + rel->name() +
					"\"? Its recordings stay in the library without a release.")) {
			m_db->deleteRelease(rel->id());
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
