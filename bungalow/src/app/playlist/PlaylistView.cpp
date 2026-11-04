#include "app/playlist/PlaylistView.h"

#include "playlist/Upsl.h"

#include "Database.h"
#include "SPTabBarElement.h"

#include <algorithm>
#include <map>

namespace space {
namespace app {
namespace {

// The ISRC of an ISRC content ref, including the legacy bungalow: forms
// (RFC-0001 §7), or "".
std::string isrcOf(const std::string& ref) {
    for (const char* prefix : { "musik:isrc:", "bungalow:recording:isrc:", "bungalow:isrc:" }) {
        const std::string p = prefix;
        if (ref.rfind(p, 0) == 0) return ref.substr(p.size());
    }
    return {};
}

// key -> percent-decoded value of a musik:artist:<a>:release:<r>[:track:<n>]
// :name:<n>[:version:<v>] ref (RFC-0002 §6); empty for any other ref.
std::map<std::string, std::string> nameRefFields(const std::string& ref) {
    std::map<std::string, std::string> fields;
    const std::string prefix = "musik:";
    if (ref.rfind(prefix + "artist:", 0) != 0) return fields;
    std::vector<std::string> parts;
    size_t start = prefix.size();
    while (start <= ref.size()) {
        const size_t colon = std::min(ref.find(':', start), ref.size());
        parts.push_back(ref.substr(start, colon - start));
        start = colon + 1;
    }
    for (size_t i = 0; i + 1 < parts.size(); i += 2)
        fields[parts[i]] = playlist::percentDecode(parts[i + 1]);
    return fields;
}

} // namespace

PlaylistView::PlaylistView(SPElement* parent, Database* db) : MusicBrowseView(parent, db) {
    addPage("tracks", "Playlist", "", {"#", "Name", "Artists"});
}

bool PlaylistView::acceptsUri(const std::string& uri) {
    return playlist::Upsl::isUpsl(uri);
}

void PlaylistView::navigate(const std::string& uri) {
    m_uri = uri;
    setTabBaseUri(uri);
    MusicBrowseView::navigate(uri);
}

void PlaylistView::reload() {
    std::string heading = "Playlist not found";
    std::vector<MusicEntityRow> rows;
    if (const auto upsl = playlist::Upsl::parse(m_uri)) {
        // The user's own playlists are in the database; anything else is shared.
        const std::string localName = m_db->getPlaylistName(upsl->playlistId);
        std::string name = upsl->embedded ? upsl->name : localName;
        if (upsl->embedded && name.empty()) name = "Untitled playlist";
        if (!name.empty()) {
            heading = name;
            if (localName.empty()) heading += " \xe2\x80\x94 shared by " + upsl->handle;  // —
        }
        for (size_t i = 0; i < upsl->refs.size(); ++i)
            rows.push_back(rowFor(static_cast<int>(i), upsl->refs[i]));
    }
    this->tabBar->getTabs().front()->name = heading;
    m_pages.front()->source->setRows(std::move(rows));
    this->tabBar->invalidate();
}

MusicEntityRow PlaylistView::rowFor(int index, const std::string& ref) const {
    MusicEntityRow row;
    row.key = ref;
    row.title = ref;
    row.playable = false;
    const std::string number = std::to_string(index + 1);

    const std::string isrc = isrcOf(ref);
    if (!isrc.empty()) {
        const auto tracks = m_db->getTracksForRecording(isrc);
        if (!tracks.empty()) {
            const Database::TrackRow& track = tracks.front();
            row.title = track.name;
            row.artist = track.artists;
            row.trackId = track.id;
            row.playUri = track.playUri;
            row.playable = !track.playUri.empty();
        } else {
            row.title = isrc;
        }
    } else if (const auto fields = nameRefFields(ref); !fields.empty()) {
        const auto field = [&fields](const char* key) {
            const auto found = fields.find(key);
            return found == fields.end() ? std::string() : found->second;
        };
        row.title = field("name");
        if (!field("version").empty()) row.title += " (" + field("version") + ")";
        row.artist = field("artist");
    }
    row.cells = {number, row.title, row.artist};
    return row;
}

void PlaylistView::onActivate(Page& page, int row) {
    playFrom(page, row);
}

} // namespace app
} // namespace space
