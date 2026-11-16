#pragma once

#include "feature/Feature.h"

#include <shore/feature_manager.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace space {
class SPViewElement;
class SPViewStackElement;

namespace feature {

/// Shore's registry of features, wired to Space: registering a feature
/// mounts its views in the view stack; unregistering it takes them out again.
class FeatureManager : public shore::SHFeatureManager {
public:
    explicit FeatureManager(SPViewStackElement* viewStack);

    /// Takes ownership of the feature and returns it. A feature with the same
    /// id is unregistered first.
    Feature* registerAspect(std::unique_ptr<Feature> feature);
    void unregisterAspect(const std::string& id) { Unregister(id); }
    Feature* get(const std::string& id) const { return dynamic_cast<Feature*>(Get(id)); }
    /// Every registered feature, in registration order.
    std::vector<Feature*> all() const;

    /// Sidebar nodes of every feature, in registration order.
    std::vector<navigation::SidebarNode> sidebarNodes() const;

protected:
    void OnFeatureRegistered(shore::SHFeature& feature) override;
    void OnFeatureUnregistering(shore::SHFeature& feature) override;

private:
    SPViewStackElement* m_viewStack;
    std::map<std::string, std::vector<SPViewElement*>> m_views;
};

} // namespace feature
} // namespace space
