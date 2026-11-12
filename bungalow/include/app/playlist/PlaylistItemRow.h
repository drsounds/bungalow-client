#pragma once

#include "app/library/MusicEntity.h"

#include <memory>
#include <string>

namespace space {
namespace app {

/**
 * PlaylistItemRow — one item of a playlist, at its position.
 *
 * It wraps the entity the item refers to, which can be any kind of Entity:
 * a track, a release, an artist, or a kind added later. The playlist stores
 * only the entity's URI, so that is all a kind needs in order to be listed.
 * What the row shows and plays is asked of the entity: every entity has a
 * name, a MediaEntity can also play, and a MusicEntity fills the music columns.
 */
class PlaylistItemRow final : public MusicEntity {
public:
    /** @p position is 0-based; @p entity must not be null. */
    PlaylistItemRow(int position, std::shared_ptr<const Entity> entity);

    const std::shared_ptr<const Entity>& entity() const { return m_entity; }
    int position() const { return m_position; }

    /** The item's identity and what the playlist stores: the entity's URI. */
    std::string id() const override { return m_entity->uri(); }
    std::string uri() const override { return m_entity->uri(); }
    std::string name() const override { return m_entity->name(); }

    std::string playUri() const override;
    std::string artistNames() const override;
    int64_t     trackId() const override;
    std::string isrc() const override;
    /** "#" is the place in the playlist; other columns are the entity's. */
    std::string text(MusicColumn column) const override;

private:
    int                           m_position = 0;
    std::shared_ptr<const Entity> m_entity;
    // The same entity seen as what it also is, nullptr when it isn't.
    const MediaEntity*            m_media = nullptr;
    const MusicEntity*            m_music = nullptr;
};

/**
 * UnresolvedEntity — an entity known only by its URI, for a playlist item
 * the library has nothing for. It is listed by whatever name the URI gives.
 */
class UnresolvedEntity final : public MediaEntity {
public:
    UnresolvedEntity(std::string uri, std::string name, std::string artists = {});

    std::string id() const override { return m_uri; }
    std::string uri() const override { return m_uri; }
    std::string name() const override { return m_name; }
    std::string artistNames() const override { return m_artists; }

private:
    std::string m_uri;
    std::string m_name;
    std::string m_artists;
};

} // namespace app
} // namespace space
