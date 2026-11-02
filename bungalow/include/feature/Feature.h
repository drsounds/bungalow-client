#pragma once

#include "aspect/Aspect.h"
#include "navigation/SidebarNode.h"

#include "SPViewStackElement.h"

#include <vector>

namespace space {
class SPViewElement;

namespace feature {

/// A few screens plus their sidebar navigation. Registered with a
/// FeatureManager, directly or through ConceptContext::registerFeature().
class Feature : public aspect::Aspect {
public:
    /// Views to mount in the view stack when the feature is registered.
    /// Create them with viewStack as their parent.
    virtual std::vector<SPViewElement*> createViews(SPViewStackElement* /*viewStack*/) { return {}; }

    /// Sidebar nodes contributed by this feature. Called every time the
    /// sidebar is rebuilt, so features with live content (bookmarks, custom
    /// tables) return their current state.
    virtual std::vector<navigation::SidebarNode> getSidebarNodes() const { return {}; }
};

} // namespace feature
} // namespace space
