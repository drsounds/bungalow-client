#pragma once

#include "SPViewElement.h"

#include <string>

namespace space {
class SPHeaderElement;

namespace app {

/// bungalow:playlist:<id> — the playlist's name.
class PlaylistView : public SPViewElement {
public:
    explicit PlaylistView(SPElement* parent);

    bool acceptsUri(const std::string& uri) override;
    void navigate(const std::string& uri) override;
    /// Shows the current playlist's name again, e.g. after a rename.
    void refresh();

private:
    SPHeaderElement* m_header = nullptr;
    std::string m_uri;
};

} // namespace app
} // namespace space
