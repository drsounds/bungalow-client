#include "RecordingsView.h"

#include "../../space/src/SPTabBarElement.h"
#include "Database.h"
#include "MusicEditActions.h"

#include <unordered_map>

namespace space {
namespace views {

RecordingsView::RecordingsView(SPElement *parent, Database *db) :
	MusicBrowseView(parent, db) {
	addPage("recordings", "Recordings", "bungalow:library:recordings",
			{"Name", "Version", "Artists", "ISRC"});
	this->tabBar->addToolButton("add", "+");
}

bool RecordingsView::acceptsUri(const std::string &uri) {
	return uri == "bungalow:library:recordings";
}

void RecordingsView::reload() {
	// A recording plays through the first of its tracks that has a playable.
	std::unordered_map<std::string, Database::TrackRow> playableTrack;
	for (const auto &t : m_db->getAllTracks())
		if (!t.playUri.empty())
			playableTrack.emplace(t.isrc, t);

	std::vector<MusicEntityRow> rows;
	for (const auto &rec : m_db->getRecordingSummaries()) {
		MusicEntityRow r;
		r.key = rec.recording.isrc;
		r.title = rec.recording.name;
		r.artist = rec.artists;
		r.cells = {rec.recording.name, rec.recording.version, rec.artists,
				   displayId(rec.recording.isrc)};
		auto it = playableTrack.find(r.key);
		r.playable = it != playableTrack.end();
		if (r.playable) {
			r.trackId = it->second.id;
			r.playUri = it->second.playUri;
		}
		rows.push_back(std::move(r));
	}
	m_pages.front()->source->setRows(std::move(rows));
}

void RecordingsView::onActivate(Page &page, int row) {
	playFrom(page, row);
}

void RecordingsView::onContextMenu(Page &page, int row) {
	const MusicEntityRow rec = *page.source->rowAt(row);
	enum { ID_PLAY = 10500, ID_OPEN, ID_EDIT, ID_DELETE };
	switch (popupMenu({{ID_PLAY, "Play"},
					   {ID_OPEN, "Go to Recording"},
					   {0, ""},
					   {ID_EDIT, "Edit..."},
					   {0, ""},
					   {ID_DELETE, "Delete Recording"}})) {
	case ID_PLAY:
		playFrom(page, row);
		break;
	case ID_OPEN:
		open(entityUri("recording", rec.key));
		break;
	case ID_EDIT:
		if (!edit::recording(topWindow(), *m_db, rec.key).empty())
			refresh();
		break;
	case ID_DELETE:
		if (confirm("Delete the recording \"" + rec.title +
					"\"? Its tracks and library saves are deleted too.")) {
			m_db->deleteRecording(rec.key);
			refresh();
		}
		break;
	default:
		break;
	}
}

void RecordingsView::onToolButton(const std::string &id) {
	if (id == "add" && !edit::recording(topWindow(), *m_db).empty())
		refresh();
}

} // namespace views
} // namespace space
