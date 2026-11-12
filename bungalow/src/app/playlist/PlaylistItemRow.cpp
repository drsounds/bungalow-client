#include "app/playlist/PlaylistItemRow.h"

#include <utility>

namespace space {
namespace app {

PlaylistItemRow::PlaylistItemRow(int position, std::shared_ptr<const Entity> entity)
    : m_position(position), m_entity(std::move(entity)),
      m_media(dynamic_cast<const MediaEntity*>(m_entity.get())),
      m_music(dynamic_cast<const MusicEntity*>(m_entity.get())) {}

std::string PlaylistItemRow::playUri() const {
    return m_media ? m_media->playUri() : "";
}

std::string PlaylistItemRow::artistNames() const {
    return m_media ? m_media->artistNames() : "";
}

int64_t PlaylistItemRow::trackId() const {
    return m_media ? m_media->trackId() : 0;
}

std::string PlaylistItemRow::isrc() const {
    return m_media ? m_media->isrc() : "";
}

std::string PlaylistItemRow::text(MusicColumn column) const {
    switch (column) {
    case MusicColumn::Number:  return std::to_string(m_position + 1);
    case MusicColumn::Name:    return name();
    case MusicColumn::Artists: return artistNames();
    default:                   return m_music ? m_music->text(column) : "";
    }
}

UnresolvedEntity::UnresolvedEntity(std::string uri, std::string name, std::string artists)
    : m_uri(std::move(uri)), m_name(std::move(name)), m_artists(std::move(artists)) {}

} // namespace app
} // namespace space
