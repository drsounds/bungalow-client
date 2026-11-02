#pragma once

#include "concept/Concept.h"

namespace space {
namespace app {

/// The unified search screen (bungalow:search:<query>). It shows the results
/// of every registered SearchProvider; other concepts make their entities
/// searchable by registering providers.
class SearchConcept : public concepts::Concept {
public:
    std::string getId() const override { return "search"; }
    std::string getName() const override { return "Search"; }
    void onActivate(concepts::ConceptContext& ctx) override;
};

} // namespace app
} // namespace space
