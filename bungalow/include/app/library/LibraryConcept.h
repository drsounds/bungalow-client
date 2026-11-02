#pragma once

#include "concept/Concept.h"

namespace space {
namespace app {

/// The local library: the Tracks and Local Files views with their sidebar
/// entries, and the library search provider.
class LibraryConcept : public concepts::Concept {
public:
    std::string getId() const override { return "library"; }
    std::string getName() const override { return "Library"; }
    std::string getIconPath() const override { return "icons/Library.svg"; }
    void onActivate(concepts::ConceptContext& ctx) override;
};

} // namespace app
} // namespace space
