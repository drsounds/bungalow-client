#pragma once

#include "search/SearchProvider.h"

namespace space {
namespace app {

/// Searches the saved tracks of the local library by song, artist and release
/// name. Results are bungalow:track:<id> URIs, which play the track.
class LibrarySearchProvider : public search::SearchProvider {
public:
    std::string getId() const override { return "search.library"; }
    std::string getName() const override { return "Library"; }
    std::vector<search::SearchResult> search(const std::string& query) override;
};

} // namespace app
} // namespace space
