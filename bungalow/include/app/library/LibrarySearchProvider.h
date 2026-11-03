#pragma once

#include "search/SearchProvider.h"

namespace space {
namespace app {

/// Searches the local library: artists, releases and recordings by name (or
/// exact ISNI/UPC/ISRC), which open their bungalow:library:<kind>:<key> detail
/// pages, and saved tracks by song, artist and release name, whose
/// bungalow:track:<id> URIs play the track.
class LibrarySearchProvider : public search::SearchProvider {
public:
    std::string getId() const override { return "search.library"; }
    std::string getName() const override { return "Library"; }
    std::vector<search::SearchResult> search(const std::string& query) override;
};

} // namespace app
} // namespace space
