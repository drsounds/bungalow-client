#include "app/playlist/PlaylistView.h"

#include "app/library/MusicEditActions.h"
#include "app/library/MusicEntityRepository.h"
#include "app/playlist/PlaylistItemRow.h"
#include "playlist/Upsl.h"
#include "playlist/RootlistManager.h"

#include "Database.h"
#include "BungalowApp.h"
#include "SPTabBarElement.h"

#include <algorithm>
#include <limits>
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

// The entity a playlist's stored URI refers to. What the library has becomes
// that entity; anything else is listed by the name its URI carries.
std::shared_ptr<const Entity> entityFor(const MusicEntityRepository& library, const std::string& ref) {
    const std::string isrc = isrcOf(ref);
    if (!isrc.empty()) {
        const auto tracks = library.tracksForRecording(isrc);
        if (!tracks.empty()) return tracks.front();
        return std::make_shared<UnresolvedEntity>(ref, isrc);
    }
    if (ref.rfind("musik:upc:", 0) == 0) {
        if (auto release = library.release(ref.substr(std::string("musik:upc:").size())))
            return release;
    } else if (ref.rfind("musik:isni:", 0) == 0) {
        if (auto artist = library.artist(ref.substr(std::string("musik:isni:").size())))
            return artist;
    } else if (const auto fields = nameRefFields(ref); !fields.empty()) {
        const auto field = [&fields](const char* key) {
            const auto found = fields.find(key);
            return found == fields.end() ? std::string() : found->second;
        };
        std::string name = field("name");
        if (!field("version").empty()) name += " (" + field("version") + ")";
        return std::make_shared<UnresolvedEntity>(ref, std::move(name), field("artist"));
    }
    return std::make_shared<UnresolvedEntity>(ref, ref);
}

} // namespace

PlaylistView::PlaylistView(SPElement* parent, Database* db) : MusicBrowseView(parent, db) {
    addPage("tracks", "Playlist", "", {MusicColumn::Number, MusicColumn::Name, MusicColumn::Artists});
    this->tabBar->addToolButton("add", "+");
}

void PlaylistView::onToolButton(const std::string& id) {
    if (id != "add" || !m_db)
        return;
    auto* app = BungalowApp::current();
    if (!acceptsDrop()) {
        if (app) app->showError("Entries can only be added to your own playlists.");
        return;
    }
    const std::string ref = views::edit::playlistEntry(*m_db);
    if (ref.empty())
        return;
    if (onDropRefs({ref}, std::numeric_limits<int>::max()))
        refresh();
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
            items.push_back(std::make_shared<PlaylistItemRow>(
                static_cast<int>(i), entityFor(library, refs[i])));
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

bool PlaylistView::onMoveRows(const std::vector<int>& rows, int position, std::vector<int>& newRows) {
    const auto upsl = playlist::Upsl::parse(m_uri);
    if (!m_db || !upsl || !acceptsDrop())
        return false;
    std::vector<std::string> refs;
    if (!m_db->getPlaylistRefs(upsl->playlistId, refs))
        return false;

    std::vector<int> sorted;
    for (int r : rows)
        if (r >= 0 && r < static_cast<int>(refs.size()))
            sorted.push_back(r);
    std::sort(sorted.begin(), sorted.end());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
    if (sorted.empty())
        return false;

    position = std::clamp(position, 0, static_cast<int>(refs.size()));
    std::vector<std::string> moved, rest;
    int insertAt = 0;
    for (int i = 0; i < static_cast<int>(refs.size()); ++i) {
        if (std::binary_search(sorted.begin(), sorted.end(), i)) {
            moved.push_back(refs[i]);
        } else {
            if (i < position) ++insertAt;
            rest.push_back(refs[i]);
        }
    }
    rest.insert(rest.begin() + insertAt, moved.begin(), moved.end());
    for (int i = 0; i < static_cast<int>(moved.size()); ++i)
        newRows.push_back(insertAt + i);
    if (rest == refs)
        return true;
    if (!m_db->setPlaylistRefs(upsl->playlistId, rest)) {
        if (auto* app = BungalowApp::current())
            app->showError("Could not reorder this playlist.");
        return false;
    }
    return true;
}

} // namespace app
} // namespace space
