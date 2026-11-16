#include "concept/ConceptManager.h"

#include "concept/ConceptContext.h"
#include "feature/FeatureManager.h"
#include "search/SearchManager.h"

#include "SPViewElement.h"
#include "SPViewStackElement.h"

#include <vector>

namespace space {
namespace concepts {

/// Forwards contributions to the host and remembers them for Undo(). Features
/// are recorded by the Shore base class.
class ConceptManager::RecordingContext : public ConceptContext {
public:
    RecordingContext(std::string conceptId, const Host& host)
        : ConceptContext(std::move(conceptId), host.features), m_host(host) {}

    feature::Feature* registerFeature(std::unique_ptr<feature::Feature> feature) override {
        if (!feature || !m_host.features) return nullptr;
        feature::Feature* raw = feature.get();
        if (!RegisterFeature(std::move(feature))) return nullptr;
        refreshSidebar();
        return raw;
    }

    void registerView(SPViewElement* view) override {
        if (!view || !m_host.viewStack) return;
        m_host.viewStack->appendChild(view);
        m_views.push_back(view);
    }

    search::SearchProvider* registerSearchProvider(std::unique_ptr<search::SearchProvider> provider) override {
        if (!provider || !m_host.search) return nullptr;
        m_providers.push_back(provider->getId());
        return m_host.search->registerAspect(std::move(provider));
    }

    SPViewStackElement* viewStack() override { return m_host.viewStack; }
    search::SearchManager& searchManager() override { return *m_host.search; }

    void refreshSidebar() override {
        if (m_host.refreshSidebar) m_host.refreshSidebar();
    }

    /// Removes every contribution, newest first.
    void Undo() override {
        for (auto it = m_providers.rbegin(); it != m_providers.rend(); ++it)
            m_host.search->unregisterAspect(*it);
        ConceptContext::Undo(); // the features
        // Space elements are never freed (see FeatureManager::OnFeatureUnregistering).
        for (auto it = m_views.rbegin(); it != m_views.rend(); ++it) {
            m_host.viewStack->removeChild(*it);
            (*it)->setVisible(false);
        }
        if (!m_views.empty()) m_host.viewStack->pack();
        m_providers.clear();
        m_views.clear();
        refreshSidebar();
    }

private:
    const Host& m_host;
    std::vector<std::string> m_providers;
    std::vector<SPViewElement*> m_views;
};

ConceptManager::ConceptManager(Host host)
    : shore::SHConceptManager(host.features), m_host(std::move(host)) {}

std::unique_ptr<shore::SHConceptContext> ConceptManager::CreateContext(const std::string& conceptId) {
    return std::make_unique<RecordingContext>(conceptId, m_host);
}

Concept* ConceptManager::registerAspect(std::unique_ptr<Concept> item) {
    Concept* raw = item.get();
    return Register(std::move(item)) ? raw : nullptr;
}

std::vector<Concept*> ConceptManager::all() const {
    std::vector<Concept*> result;
    for (shore::SHConcept* item : All())
        if (auto* registered = dynamic_cast<Concept*>(item)) result.push_back(registered);
    return result;
}

} // namespace concepts
} // namespace space
