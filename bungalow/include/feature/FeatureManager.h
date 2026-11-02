#pragma once

#include "aspect/BaseAspectManager.h"
#include "feature/Feature.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace space {
class SPViewElement;
class SPViewStackElement;

namespace feature {

/// Registry of features. Registering a feature mounts its views in the view
/// stack; unregistering it takes them out again.
class FeatureManager : public aspect::BaseAspectManager<Feature> {
public:
    explicit FeatureManager(SPViewStackElement* viewStack);

    Feature* registerAspect(std::unique_ptr<Feature> feature) override;
    void unregisterAspect(const std::string& id) override;

    /// Sidebar nodes of every feature, in registration order.
    std::vector<navigation::SidebarNode> sidebarNodes() const;

private:
    SPViewStackElement* m_viewStack;
    std::map<std::string, std::vector<SPViewElement*>> m_views;
};

} // namespace feature
} // namespace space
