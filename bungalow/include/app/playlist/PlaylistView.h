#pragma once

#include "app/library/MusicBrowseView.h"

#include <string>

class Database;

namespace space {
namespace app {

/// A federated playlist, musik:federated:user:<handle>:playlist:<uuid>[:<body>]
/// (RFC-0001). A link with an embedded body shows the playlist it carries;
/// a hosted link shows the user's own playlist with that id from the
/// database. Content refs are listed as tracks, playable when an ISRC is in
/// the library.
class PlaylistView : public views::MusicBrowseView {
public:
    PlaylistView(SPElement* parent, Database* db);

    bool acceptsUri(const std::string& uri) override;
    void navigate(const std::string& uri) override;
    /// Shows the current playlist again, e.g. after a rename.
    using MusicBrowseView::refresh;

protected:
    void reload() override;
    void onActivate(Page& page, int row) override;

private:
    MusicEntityRow rowFor(int index, const std::string& ref) const;

    std::string m_uri;
};

} // namespace app
} // namespace space
