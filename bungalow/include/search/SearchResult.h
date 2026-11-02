#pragma once

#include "search/EntityKind.h"

#include <string>

namespace space {
namespace search {

/// One hit returned by a SearchProvider. uri is activated on click: played if
/// the application can play it (e.g. bungalow:track:<id>), otherwise
/// navigated to. source names the contributing provider (Library, …).
struct SearchResult {
    EntityKind kind = EntityKind::Other;
    std::string title;
    std::string subtitle;
    std::string uri;
    std::string source;
};

} // namespace search
} // namespace space
