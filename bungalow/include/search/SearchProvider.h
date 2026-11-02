#pragma once

#include "aspect/Aspect.h"
#include "search/SearchResult.h"

#include <string>
#include <vector>

namespace space {
namespace search {

/// A pluggable source of search results: library entries, catalogue hits, …
/// Registered through ConceptContext::registerSearchProvider(); its results
/// appear in the unified search view. getName() is the source label shown on
/// each result.
class SearchProvider : public aspect::Aspect {
public:
    /// Results matching query. Called on the UI thread, so keep it quick.
    virtual std::vector<SearchResult> search(const std::string& query) = 0;
};

} // namespace search
} // namespace space
