#include "feature/FeatureManager.h"

#include "SPViewElement.h"
#include "SPViewStackElement.h"

namespace space {
namespace feature {

FeatureManager::FeatureManager(SPViewStackElement* viewStack) : m_viewStack(viewStack) {}

Feature* FeatureManager::registerAspect(std::unique_ptr<Feature> feature) {
    Feature* registered = BaseAspectManager::registerAspect(std::move(feature));
    if (!registered || !m_viewStack) return registered;

    auto& views = m_views[registered->getId()];
    for (SPViewElement* view : registered->createViews(m_viewStack)) {
        if (!view) continue;
        m_viewStack->appendChild(view);
        views.push_back(view);
    }
    return registered;
}

void FeatureManager::unregisterAspect(const std::string& id) {
    auto it = m_views.find(id);
    if (it != m_views.end()) {
        // Space elements are never freed: other elements may still hold
        // listeners on them. Detached and hidden, the views are inert.
        for (SPViewElement* view : it->second) {
            m_viewStack->removeChild(view);
            view->setVisible(false);
        }
        m_views.erase(it);
        m_viewStack->pack();
    }
    BaseAspectManager::unregisterAspect(id);
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
