#pragma once

namespace space {
namespace concepts { class ConceptManager; }
namespace feature { class FeatureManager; }

namespace app {

/// Registers the built-in features and concepts. Registration order is the
/// order of their sidebar sections and of their views in the view stack.
void registerBuiltins(feature::FeatureManager& features, concepts::ConceptManager& concepts);

} // namespace app
} // namespace space
