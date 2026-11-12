#include "app/library/RecordingsLibraryView.h"

#include "SPTabBarElement.h"
#include "Database.h"
#include "PlatformUi.h"
#include "app/library/MusicEditActions.h"
#include "app/library/MusicEntityRepository.h"

namespace space {
namespace views {

RecordingsLibraryView::RecordingsLibraryView(SPElement *parent, Database *db) :
	MusicBrowseView(parent, db) {
	addPage("recordings", "Recordings", "bungalow:library:recordings",
			{MusicColumn::Name, MusicColumn::Version, MusicColumn::Artists, MusicColumn::Isrc});
	this->tabBar->addToolButton("add", "+");
}

bool RecordingsLibraryView::acceptsUri(const std::string &uri) {
	return uri == "bungalow:library:recordings";
}

void RecordingsLibraryView::reload() {
	m_pages.front()->source->setEntities(MusicEntityRepository(*m_db).recordings());
}

void RecordingsLibraryView::onActivate(Page &page, int row) {
	playFrom(page, row);
}

void RecordingsLibraryView::onContextMenu(Page &page, int row) {
	// Holding the entity keeps it alive when refresh() replaces the rows.
	const auto rec = page.source->rowAt(row);
	enum { ID_PLAY = 10500, ID_OPEN, ID_EDIT, ID_DELETE };
	switch (platform::showPopupMenu({{ID_PLAY, "Play"},
					   {ID_OPEN, "Go to Recording"},
					   {0, ""},
					   {ID_EDIT, "Edit..."},
					   {0, ""},
					   {ID_DELETE, "Delete Recording"}})) {
	case ID_PLAY:
		playFrom(page, row);
		break;
	case ID_OPEN:
		open(libraryUri("recording", rec->id()));
		break;
	case ID_EDIT:
		if (!edit::recording(*m_db, rec->id()).empty())
			refresh();
		break;
	case ID_DELETE:
		if (platform::showConfirmDialog("Confirm Delete", "Delete the recording \"" + rec->name() +
					"\"? Its tracks and library saves are deleted too.")) {
			m_db->deleteRecording(rec->id());
			refresh();
		}
		break;
	default:
		break;
	}
}

void RecordingsLibraryView::onToolButton(const std::string &id) {
	if (id == "add" && !edit::recording(*m_db).empty())
		refresh();
}

} // namespace views
} // namespace space
