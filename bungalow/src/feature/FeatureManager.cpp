#include "feature/FeatureManager.h"

#include "SPViewElement.h"
#include "SPViewStackElement.h"

namespace space {
namespace feature {

FeatureManager::FeatureManager(SPViewStackElement* viewStack) : m_viewStack(viewStack) {}

Feature* FeatureManager::registerAspect(std::unique_ptr<Feature> feature) {
    Feature* raw = feature.get();
    return Register(std::move(feature)) ? raw : nullptr;
}

std::vector<Feature*> FeatureManager::all() const {
    std::vector<Feature*> result;
    for (shore::SHFeature* item : All())
        if (auto* feature = dynamic_cast<Feature*>(item)) result.push_back(feature);
    return result;
}

void FeatureManager::OnFeatureRegistered(shore::SHFeature& registered) {
    auto* feature = dynamic_cast<Feature*>(&registered);
    if (!feature || !m_viewStack) return;

    auto& views = m_views[feature->getId()];
    for (SPViewElement* view : feature->createViews(m_viewStack)) {
        if (!view) continue;
        m_viewStack->appendChild(view);
        views.push_back(view);
    }
}

void FeatureManager::OnFeatureUnregistering(shore::SHFeature& feature) {
    auto it = m_views.find(feature.Id());
    if (it == m_views.end()) return;
    // Space elements are never freed: other elements may still hold
    // listeners on them. Detached and hidden, the views are inert.
    for (SPViewElement* view : it->second) {
        m_viewStack->removeChild(view);
        view->setVisible(false);
    }
    m_views.erase(it);
    m_viewStack->pack();
}

std::vector<navigation::SidebarNode> FeatureManager::sidebarNodes() const {
    std::vector<navigation::SidebarNode> nodes;
    for (Feature* feature : all()) {
        for (auto& node : feature->getSidebarNodes())
            nodes.push_back(std::move(node));
    }
    return nodes;
}

} // namespace feature
} // namespace space
