#include "search/SearchManager.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace space;

namespace {

int failures = 0;

void check(bool ok, const char* what) {
    if (ok) return;
    std::cerr << what << "\n";
    ++failures;
}

class FakeProvider : public search::SearchProvider {
public:
    FakeProvider(std::string id, std::vector<search::SearchResult> results, bool throws = false)
        : m_id(std::move(id)), m_results(std::move(results)), m_throws(throws) {}

    std::string getId() const override { return m_id; }
    std::string getName() const override { return "Fake " + m_id; }
    void onRegister(aspect::AspectManagerBase&) override { ++registrations; }

    std::vector<search::SearchResult> search(const std::string&) override {
        if (m_throws) throw std::runtime_error("provider failed");
        return m_results;
    }

    int registrations = 0;

private:
    std::string m_id;
    std::vector<search::SearchResult> m_results;
    bool m_throws;
};

search::SearchResult result(search::EntityKind kind, const std::string& title, const std::string& source = {}) {
    search::SearchResult r;
    r.kind = kind;
    r.title = title;
    r.source = source;
    return r;
}

} // namespace

int main() {
    search::SearchManager manager;

    auto* a = manager.registerAspect(std::make_unique<FakeProvider>("a", std::vector<search::SearchResult>{
        result(search::EntityKind::Track, "a-track"),
        result(search::EntityKind::Artist, "a-artist", "Custom"),
    }));
    manager.registerAspect(std::make_unique<FakeProvider>("broken", std::vector<search::SearchResult>{}, true));
    manager.registerAspect(std::make_unique<FakeProvider>("b", std::vector<search::SearchResult>{
        result(search::EntityKind::Other, "b-other"),
        result(search::EntityKind::Artist, "b-artist"),
    }));

    check(a != nullptr && static_cast<FakeProvider*>(a)->registrations == 1, "onRegister is called once");
    check(manager.all().size() == 3, "three providers registered");
    check(manager.get("b") != nullptr && manager.get("missing") == nullptr, "get by id");

    const auto results = manager.search("q");
    const std::vector<std::string> expected = {"a-artist", "b-artist", "a-track", "b-other"};
    check(results.size() == expected.size(), "a throwing provider is skipped");
    for (size_t i = 0; i < results.size() && i < expected.size(); ++i)
        check(results[i].title == expected[i], "results are ordered by kind, then provider");
    check(!results.empty() && results[0].source == "Custom", "a provider's source is kept");
    check(results.size() > 1 && results[1].source == "Fake b", "an empty source becomes the provider name");
    check(manager.search("  ").empty(), "a blank query returns nothing");

    // Re-registering an id replaces the old aspect and moves it to the end.
    manager.registerAspect(std::make_unique<FakeProvider>("a", std::vector<search::SearchResult>{}));
    const auto providers = manager.providers();
    check(providers.size() == 3 && providers.back()->getId() == "a", "re-registering replaces by id");

    manager.unregisterAspect("broken");
    manager.unregisterAspect("missing");
    check(manager.all().size() == 2 && manager.get("broken") == nullptr, "unregister by id");

    return failures == 0 ? 0 : 1;
}
