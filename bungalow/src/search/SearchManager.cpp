#include "search/SearchManager.h"

#include <algorithm>
#include <cctype>
#include <exception>

namespace space {
namespace search {

std::vector<SearchResult> SearchManager::search(const std::string& query) const {
    std::vector<SearchResult> results;
    const bool blank = std::all_of(query.begin(), query.end(),
                                   [](unsigned char c) { return std::isspace(c) != 0; });
    if (blank) return results;

    for (SearchProvider* provider : providers()) {
        try {
            for (auto& result : provider->search(query)) {
                if (result.source.empty()) result.source = provider->getName();
                results.push_back(std::move(result));
            }
        } catch (const std::exception&) {
            // One provider failing must not hide the others' results.
        }
    }
    std::stable_sort(results.begin(), results.end(), [](const SearchResult& a, const SearchResult& b) {
        return order(a.kind) < order(b.kind);
    });
    return results;
}

} // namespace search
} // namespace space
