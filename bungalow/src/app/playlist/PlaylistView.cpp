#include "app/playlist/PlaylistView.h"

#include "app/library/MusicEntityRepository.h"
#include "playlist/Upsl.h"
#include "playlist/RootlistManager.h"

#include "Database.h"
#include "BungalowApp.h"
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

// One content ref of a playlist, at its position. A ref the library can
// resolve to a track plays through it; any other ref is listed by name only.
class PlaylistItemEntity final : public MusicEntity {
public:
    PlaylistItemEntity(int position, std::string ref, std::string name, std::string artists,
                       std::shared_ptr<const TrackEntity> track)
        : m_position(position), m_ref(std::move(ref)), m_name(std::move(name)),
          m_artists(std::move(artists)), m_track(std::move(track)) {}

    std::string id() const override { return m_ref; }
    std::string name() const override { return m_name; }
    std::string uri() const override { return m_ref; }
    std::string playUri() const override { return m_track ? m_track->playUri() : ""; }
    std::string artistNames() const override { return m_artists; }
    int64_t     trackId() const override { return m_track ? m_track->trackId() : 0; }

    std::string text(MusicColumn column) const override {
        // "#" is the item's place in the playlist, not on its release.
        if (column == MusicColumn::Number) return std::to_string(m_position + 1);
        return MusicEntity::text(column);
    }

private:
    int                                m_position = 0;
    std::string                        m_ref;
    std::string                        m_name;
    std::string                        m_artists;
    std::shared_ptr<const TrackEntity> m_track;
};

std::shared_ptr<const MusicEntity> itemFor(const MusicEntityRepository& library,
                                           int position, const std::string& ref) {
    std::string name = ref;
    std::string artists;
    std::shared_ptr<const TrackEntity> track;

    const std::string isrc = isrcOf(ref);
    if (!isrc.empty()) {
        const auto tracks = library.tracksForRecording(isrc);
        if (!tracks.empty()) {
            track = tracks.front();
            name = track->name();
            artists = track->artistNames();
        } else {
            name = isrc;
        }
    } else if (ref.rfind("musik:upc:", 0) == 0) {
        if (const auto release = library.release(ref.substr(std::string("musik:upc:").size())))
            name = release->name();
    } else if (ref.rfind("musik:isni:", 0) == 0) {
        if (const auto artist = library.artist(ref.substr(std::string("musik:isni:").size())))
            name = artist->name();
    } else if (const auto fields = nameRefFields(ref); !fields.empty()) {
        const auto field = [&fields](const char* key) {
            const auto found = fields.find(key);
            return found == fields.end() ? std::string() : found->second;
        };
        name = field("name");
        if (!field("version").empty()) name += " (" + field("version") + ")";
        artists = field("artist");
    }
    return std::make_shared<PlaylistItemEntity>(position, ref, std::move(name), std::move(artists),
                                                std::move(track));
}

} // namespace

PlaylistView::PlaylistView(SPElement* parent, Database* db) : MusicBrowseView(parent, db) {
    addPage("tracks", "Playlist", "", {MusicColumn::Number, MusicColumn::Name, MusicColumn::Artists});
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
    std::vector<std::shared_ptr<const MusicEntity>> items;
    if (const auto upsl = playlist::Upsl::parse(m_uri)) {
        // The user's own playlists are in the database; anything else is shared.
        const std::string localName = m_db->getPlaylistName(upsl->playlistId);
        std::string name = upsl->embedded ? upsl->name : localName;
        if (upsl->embedded && name.empty()) name = "Untitled playlist";
        if (!name.empty()) {
            heading = name;
            if (localName.empty()) heading += " \xe2\x80\x94 shared by " + upsl->handle;  // —
        }
        std::vector<std::string> refs = upsl->refs;
        if (!upsl->embedded && m_db && !m_db->getPlaylistRefs(upsl->playlistId, refs)) {
            if (auto* app = BungalowApp::current())
                app->showError("Could not load this playlist.");
        }
        const MusicEntityRepository library(*m_db);
        for (size_t i = 0; i < refs.size(); ++i)
            items.push_back(itemFor(library, static_cast<int>(i), refs[i]));
    }
    this->tabBar->getTabs().front()->name = heading;
    m_pages.front()->source->setEntities(items);
    this->tabBar->invalidate();
}

void PlaylistView::onActivate(Page& page, int row) {
    playFrom(page, row);
}

bool PlaylistView::acceptsDrop() const {
    const auto upsl = playlist::Upsl::parse(m_uri);
    return m_db && upsl && playlist::RootlistManager::isLocalPlaylist(*m_db, m_uri);
}

bool PlaylistView::onDropRefs(const std::vector<std::string>& refs, int position) {
    if (!m_db || !playlist::RootlistManager::insertContentRefs(*m_db, m_uri, refs, position)) {
        if (auto* app = BungalowApp::current())
            app->showError("Could not add items to this playlist.");
        return false;
    }
    return true;
}

} // namespace app
} // namespace space
