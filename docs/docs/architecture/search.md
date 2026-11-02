---
title: Search
sidebar_position: 5
---

# Search

Search is unified: one screen shows results from every registered **search
provider**, grouped by the kind of entity they represent. A concept makes its
entities searchable by registering a provider. The search screen doesn't know
about any specific source.

## Types

```cpp
namespace space::search {

enum class EntityKind { Artist, Release, Recording, Track, Video, Other };

struct SearchResult {
    EntityKind kind = EntityKind::Other;
    std::string title;
    std::string subtitle;
    std::string uri;      // played or navigated to on activation
    std::string source;   // the provider's name, shown on the result
};

class SearchProvider : public aspect::Aspect {
public:
    /// Called on the UI thread, so keep it quick.
    virtual std::vector<SearchResult> search(const std::string& query) = 0;
};

class SearchManager : public aspect::BaseAspectManager<SearchProvider> {
public:
    std::vector<SearchProvider*> providers() const;
    std::vector<SearchResult> search(const std::string& query) const;
};

}
```

`SearchManager::search()` runs the query against every provider:

* A blank query returns nothing.
* A provider that throws is skipped.
* Results are ordered by `EntityKind` (Artists, Releases, Songs, Tracks, Videos,
  Other) and keep provider order within a kind.

## The search view

`SearchConcept` registers `app::SearchView`, which accepts
`bungalow:search`, `bungalow:search:<query>` and `bungalow:search?q=<query>`.
It lists the results in sections headed by `label(kind)`.

Double-clicking a result **plays** it if
`SPApplicationContext::current()->canPlayUri(uri)` accepts it (for example a
library track, `bungalow:track:<id>`), and otherwise **navigates** to it.

## Writing a provider

```cpp
class MyCatalogueProvider : public search::SearchProvider {
public:
    std::string getId() const override   { return "mycatalogue"; }
    std::string getName() const override { return "My Catalogue"; }

    std::vector<search::SearchResult> search(const std::string& query) override {
        std::vector<search::SearchResult> results;
        // … look up query …
        results.push_back({search::EntityKind::Artist, "Artist name", "", "bungalow:artist:0000000121032683", getName()});
        return results;
    }
};

void MyConcept::onActivate(concepts::ConceptContext& ctx) {
    ctx.registerSearchProvider(std::make_unique<MyCatalogueProvider>());
}
```

The built-in `LibrarySearchProvider` searches the local library database.
