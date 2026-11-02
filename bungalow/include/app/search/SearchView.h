#pragma once

#include "SPTabBarViewElement.h"
#include "SPTableDataSource.h"

#include "search/SearchResult.h"

#include <string>
#include <vector>

namespace space {
class SPTableElement;
class SPTextElement;
namespace search { class SearchManager; }

namespace app {

/// Table rows for SearchView: the results, already ordered by EntityKind.
class SearchResultsDataSource : public SPTableDataSource {
public:
    void setResults(std::vector<search::SearchResult> results) { m_results = std::move(results); }
    const search::SearchResult* resultAt(int row) const;

    int getColumnCount() override { return 4; }
    int getRowCount() override { return static_cast<int>(m_results.size()); }
    std::string getTextForColumn(int column) override;
    std::string getTextForCell(int column, int row) override;
    std::string getRowUri(int row) override;
    bool isRowPlayable(int row) override;

private:
    std::vector<search::SearchResult> m_results;
};

/// The unified search screen, bungalow:search:<query> (bungalow:search?q=<query>
/// also works). Queries every registered SearchProvider and lists the results
/// grouped by EntityKind. Double-clicking a result plays it if the app can,
/// otherwise navigates to it.
class SearchView : public SPTabBarViewElement {
public:
    SearchView(SPElement* parent, search::SearchManager& searchManager);

    bool acceptsUri(const std::string& uri) override;
    void navigate(const std::string& uri) override;
    void setHash(const std::string& hash) override;
    std::string getType() override { return "searchview"; }

    /// The query of a search URI ("" for bungalow:search).
    static std::string queryFromUri(const std::string& uri);

private:
    int onRowDblClicked(SPObject* sender, SPEventArgs* args);

    search::SearchManager& m_searchManager;
    SearchResultsDataSource* m_results = nullptr;
    SPTextElement* m_heading = nullptr;
    SPTableElement* m_table = nullptr;
};

} // namespace app
} // namespace space
