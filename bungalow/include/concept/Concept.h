#pragma once

#include "aspect/Aspect.h"
#include "concept/ConceptContext.h"

#include <shore/concept.h>

#include <string>

// The namespace is "concepts": "concept" is a keyword in C++20.
namespace space {
namespace concepts {

/// A feature domain (the library, web browsing, search, …): a
/// shore::SHConcept that is activated with the client's ConceptContext. When
/// registered with the ConceptManager it contributes its features, views and
/// search providers through that context. Everything added that way is
/// removed again when the concept is unregistered.
class Concept : public aspect::AspectNames<shore::SHConcept> {
public:
    virtual void onActivate(ConceptContext& ctx) = 0;
    /// Releases whatever onActivate() acquired outside the context (listeners, …).
    virtual void onDeactivate() {}
    /// Icon shown for the concept; empty falls back to a default.
    virtual std::string getIconPath() const { return {}; }

    // Shore's spelling ends up in the functions above. A concept is only
    // activated by the client's ConceptManager, whose contexts are
    // ConceptContexts.
    void OnActivate(shore::SHConceptContext& context) final {
        if (auto* ctx = dynamic_cast<ConceptContext*>(&context)) onActivate(*ctx);
    }
    void OnDeactivate() final { onDeactivate(); }
    std::string IconPath() const final { return getIconPath(); }
};

} // namespace concepts
} // namespace space
