#pragma once

#include "aspect/Aspect.h"

#include <string>

// The namespace is "concepts": "concept" is a keyword in C++20.
namespace space {
namespace concepts {

class ConceptContext;

/// A feature domain (the library, web browsing, search, …). When registered
/// with the ConceptManager it is activated and contributes its features,
/// views and search providers through the ConceptContext. Everything added
/// that way is removed again when the concept is unregistered.
class Concept : public aspect::Aspect {
public:
    virtual void onActivate(ConceptContext& ctx) = 0;
    /// Releases whatever onActivate() acquired outside the context (listeners, …).
    virtual void onDeactivate() {}
    /// Icon shown for the concept; empty falls back to a default.
    virtual std::string getIconPath() const { return {}; }
};

} // namespace concepts
} // namespace space
