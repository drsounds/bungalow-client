#include "app/search/SearchView.h"

#include "search/SearchManager.h"

#include "SPApplicationContext.h"
#include "SPPlaybackEventArgs.h"
#include "SPSectionElement.h"
#include "SPTabBarElement.h"
#include "SPTabStackElement.h"
#include "SPTableElement.h"
#include "SPTextElement.h"
#include "SPWindowElement.h"

#include <cctype>

namespace space {
namespace app {
namespace {

const std::string kPrefix = "bungalow:search";

int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// application/x-www-form-urlencoded decoding, as for a ?q= parameter.
std::string formDecode(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '+') {
            out += ' ';
        } else if (s[i] == '%' && i + 2 < s.size() && hexValue(s[i + 1]) >= 0 && hexValue(s[i + 2]) >= 0) {
            out += static_cast<char>(hexValue(s[i + 1]) * 16 + hexValue(s[i + 2]));
            i += 2;
        } else {
            out += s[i];
        }
    }
    return out;
}

} // namespace

// ── SearchResultsDataSource ──────────────────────────────────────────────────

const search::SearchResult* SearchResultsDataSource::resultAt(int row) const {
    if (row < 0 || row >= static_cast<int>(m_results.size())) return nullptr;
    return &m_results[row];
}

std::string SearchResultsDataSource::getTextForColumn(int column) {
    static const char* kColumns[] = {"Title", "Details", "Type", "Source"};
    return column >= 0 && column < 4 ? kColumns[column] : "";
}

std::string SearchResultsDataSource::getTextForCell(int column, int row) {
    const search::SearchResult* result = resultAt(row);
    if (!result) return "";
    switch (column) {
        case 0: return result->title;
        case 1: return result->subtitle;
        case 2: return search::label(result->kind);
        case 3: return result->source;
        default: return "";
    }
}

std::string SearchResultsDataSource::getRowUri(int row) {
    const search::SearchResult* result = resultAt(row);
    return result ? result->uri : "";
}

bool SearchResultsDataSource::isRowPlayable(int row) {
    const search::SearchResult* result = resultAt(row);
    return result && !result->uri.empty();
}

// ── SearchView ───────────────────────────────────────────────────────────────

SearchView::SearchView(SPElement* parent, search::SearchManager& searchManager)
    : SPTabBarViewElement(parent), m_searchManager(searchManager) {
    auto* section = new SPSectionElement(this->tabStack);
    section->setId("overview");
    section->setStringAttribute("title", "Search");
    section->setFlex(1);

    m_heading = new SPTextElement(section);
    m_heading->setInnerText("Search");
    m_heading->setHeight(28);
    section->appendChild(m_heading);

    m_results = new SearchResultsDataSource();
    m_table = new SPTableElement(section, m_results);
    m_table->fullView = true;
    m_table->setFlex(1);
    m_table->addEventListener("rowdblclicked", reinterpret_cast<SPCallback>(&SearchView::onRowDblClicked),
                              this, m_table);
    section->appendChild(m_table);

    this->tabStack->appendChild(section);
    this->tabBar->addTab("overview", "Search", kPrefix);
    this->tabStack->setTabId("overview");
}

bool SearchView::acceptsUri(const std::string& uri) {
    if (uri.rfind(kPrefix, 0) != 0) return false;
    return uri.size() == kPrefix.size() || uri[kPrefix.size()] == ':' || uri[kPrefix.size()] == '?';
}

std::string SearchView::queryFromUri(const std::string& uri) {
    if (uri.rfind(kPrefix, 0) != 0 || uri.size() == kPrefix.size()) return "";
    const std::string rest = uri.substr(kPrefix.size());
    if (rest[0] == ':') return rest.substr(1);
    if (rest.rfind("?q=", 0) == 0) return formDecode(rest.substr(3));
    return "";
}

void SearchView::navigate(const std::string& uri) {
    const std::string query = queryFromUri(uri);
    auto results = m_searchManager.search(query);

    if (query.empty()) m_heading->setInnerText("Search");
    else if (results.empty()) m_heading->setInnerText("No results for: " + query);
    else m_heading->setInnerText("Results for: " + query);

    m_results->setResults(std::move(results));
    m_table->invalidate();
    this->pack();
}

void SearchView::setHash(const std::string& hash) {
    SPTabBarViewElement::setHash(hash.empty() ? "overview" : hash);
}

int SearchView::onRowDblClicked(SPObject* /*sender*/, SPEventArgs* args) {
    auto* rowArgs = dynamic_cast<SPTableRowEventArgs*>(args);
    if (!rowArgs) return 0;
    const search::SearchResult* result = m_results->resultAt(rowArgs->getRow());
    if (!result || result->uri.empty()) return 0;

    SPApplicationContext* context = SPApplicationContext::current();
    if (context && context->canPlayUri(result->uri)) {
        context->playUri(result->uri);
    } else if (SPWindowElement* window = getWindowElement()) {
        window->navigate(result->uri);
    }
    return 0;
}

} // namespace app
} // namespace space
