#pragma once

#include "aspect/BaseAspectManager.h"
#include "search/SearchProvider.h"

#include <string>
#include <vector>

namespace space {
namespace search {

/// Registry of the SearchProviders contributed by concepts. The search view
/// queries every provider and groups their results by EntityKind.
class SearchManager : public aspect::BaseAspectManager<SearchProvider> {
public:
    /// All registered providers, in registration order.
    std::vector<SearchProvider*> providers() const { return all(); }

    /// Runs query against every provider. Results are ordered by EntityKind
    /// and keep provider order within a kind. A provider that throws is
    /// skipped. A blank query returns nothing.
    std::vector<SearchResult> search(const std::string& query) const;
};

} // namespace search
} // namespace space
