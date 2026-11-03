#pragma once

#include "concept/Concept.h"

namespace space {
namespace app {

/// The local library: saved Tracks, Local Files, and browse/CRUD views for
/// artists, releases, recordings and all tracks (plus their detail pages),
/// with their sidebar entries, and the library search provider.
class LibraryConcept : public concepts::Concept {
public:
    std::string getId() const override { return "library"; }
    std::string getName() const override { return "Library"; }
    std::string getIconPath() const override { return "icons/Library.svg"; }
    void onActivate(concepts::ConceptContext& ctx) override;
};

} // namespace app
} // namespace space
