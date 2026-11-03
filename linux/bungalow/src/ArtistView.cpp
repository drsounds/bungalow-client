#include "ArtistView.h"

#include "../../space/src/SPTabBarElement.h"
#include "Database.h"
#include "MusicEditActions.h"

namespace space {
namespace views {

ArtistView::ArtistView(SPElement *parent, Database *db) :
	MusicBrowseView(parent, db) {
	addPage("tracks", "Tracks", "", {"Name", "Release", "Artists", "Saved"});
	addPage("releases", "Releases", "", {"Name", "Date", "Artists", "Tracks"});
	this->tabBar->addToolButton("edit", "Edit");
	this->tabBar->addToolButton("delete", "Delete");
}

bool ArtistView::acceptsUri(const std::string &uri) {
	return !entityKey("artist", uri).empty();
}

void ArtistView::navigate(const std::string &uri) {
	m_isni = entityKey("artist", uri);
	setTabBaseUri(entityUri("artist", m_isni));
	MusicBrowseView::navigate(uri);
}

void ArtistView::reload() {
	Database::ArtistRecord artist = m_db->getArtist(m_isni);
	// The first tab carries the artist's name, as the page heading.
	this->tabBar->getTabs().front()->name = artist.isni.empty() ? "Artist not found" : artist.name;

	m_pages[0]->source->setTrackRows(
		m_db->getTracksForArtist(m_isni), [](const Database::TrackRow &t) {
			return std::vector<std::string>{t.name, t.releaseName, t.artists,
											t.saved ? "Yes" : ""};
		});

	std::vector<MusicEntityRow> releases;
	for (const auto &rel : m_db->getReleasesForArtist(m_isni)) {
		MusicEntityRow r;
		r.key = rel.release.upc;
		r.title = rel.release.name;
		r.cells = {rel.release.name, rel.release.releaseDate, rel.artists,
				   std::to_string(rel.trackCount)};
		releases.push_back(std::move(r));
	}
	m_pages[1]->source->setRows(std::move(releases));
}

void ArtistView::onActivate(Page &page, int row) {
	if (page.id == "releases")
		open(entityUri("release", page.source->rowAt(row)->key));
	else
		playFrom(page, row);
}

void ArtistView::onContextMenu(Page &page, int row) {
	if (page.id == "tracks") {
		trackContextMenu(page, row, kTrackOpenRelease | kTrackOpenRecording);
		return;
	}
	enum { ID_OPEN = 10700 };
	if (popupMenu({{ID_OPEN, "Go to Release"}}) == ID_OPEN)
		open(entityUri("release", page.source->rowAt(row)->key));
}

void ArtistView::onToolButton(const std::string &id) {
	if (id == "edit") {
		if (!edit::artist(topWindow(), *m_db, m_isni).empty())
			refresh();
	} else if (id == "delete") {
		Database::ArtistRecord artist = m_db->getArtist(m_isni);
		if (!artist.isni.empty() &&
			confirm("Delete the artist \"" + artist.name +
					"\"? Their credits are removed; recordings and releases stay.")) {
			m_db->deleteArtist(m_isni);
			open("bungalow:library:artists");
		}
	}
}

} // namespace views
} // namespace space
