#pragma once

#include "concept/Concept.h"

namespace space {
namespace app {

class PlaylistView;

/// Playlists: the rootlist at the end of the sidebar, after a gap, headed by
/// a "New playlist" entry that turns into an input for the new playlist's
/// name. Its playlists can be dragged into a new order, renamed and deleted.
/// Follows RootlistEvents to keep the sidebar current.
class PlaylistConcept : public concepts::Concept {
public:
    std::string getId() const override { return "playlist"; }
    std::string getName() const override { return "Playlists"; }
    std::string getIconPath() const override { return "icons/MusicNote.svg"; }
    void onActivate(concepts::ConceptContext& ctx) override;
    void onDeactivate() override;

private:
    concepts::ConceptContext* m_ctx = nullptr;
    PlaylistView* m_view = nullptr;
    int m_rootlistListener = 0;
};

} // namespace app
} // namespace space
