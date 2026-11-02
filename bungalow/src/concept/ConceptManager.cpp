#include "concept/ConceptManager.h"

#include "concept/ConceptContext.h"
#include "feature/FeatureManager.h"
#include "search/SearchManager.h"

#include "SPViewElement.h"
#include "SPViewStackElement.h"

#include <vector>

namespace space {
namespace concepts {

/// Forwards contributions to the host and remembers them for undo().
class ConceptManager::RecordingContext : public ConceptContext {
public:
    RecordingContext(std::string conceptId, const Host& host)
        : m_conceptId(std::move(conceptId)), m_host(host) {}

    std::string conceptId() const override { return m_conceptId; }

    feature::Feature* registerFeature(std::unique_ptr<feature::Feature> feature) override {
        if (!feature || !m_host.features) return nullptr;
        m_features.push_back(feature->getId());
        feature::Feature* registered = m_host.features->registerAspect(std::move(feature));
        refreshSidebar();
        return registered;
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
    void undo() {
        for (auto it = m_providers.rbegin(); it != m_providers.rend(); ++it)
            m_host.search->unregisterAspect(*it);
        for (auto it = m_features.rbegin(); it != m_features.rend(); ++it)
            m_host.features->unregisterAspect(*it);
        // Space elements are never freed (see FeatureManager::unregisterAspect).
        for (auto it = m_views.rbegin(); it != m_views.rend(); ++it) {
            m_host.viewStack->removeChild(*it);
            (*it)->setVisible(false);
        }
        if (!m_views.empty()) m_host.viewStack->pack();
        m_providers.clear();
        m_features.clear();
        m_views.clear();
        refreshSidebar();
    }

private:
    std::string m_conceptId;
    const Host& m_host;
    std::vector<std::string> m_features;
    std::vector<std::string> m_providers;
    std::vector<SPViewElement*> m_views;
};

ConceptManager::ConceptManager(Host host) : m_host(std::move(host)) {}

ConceptManager::~ConceptManager() {
    auto concepts = all();
    for (auto it = concepts.rbegin(); it != concepts.rend(); ++it)
        (*it)->onDeactivate();
}

Concept* ConceptManager::registerAspect(std::unique_ptr<Concept> item) {
    Concept* registered = BaseAspectManager::registerAspect(std::move(item));
    if (!registered) return nullptr;

    auto context = std::make_unique<RecordingContext>(registered->getId(), m_host);
    RecordingContext& ctx = *context;
    m_contexts[registered->getId()] = std::move(context);
    registered->onActivate(ctx);
    return registered;
}

void ConceptManager::unregisterAspect(const std::string& id) {
    if (Concept* registered = get(id)) registered->onDeactivate();
    auto it = m_contexts.find(id);
    if (it != m_contexts.end()) {
        it->second->undo();
        m_contexts.erase(it);
    }
    BaseAspectManager::unregisterAspect(id);
}

} // namespace concepts
} // namespace space
