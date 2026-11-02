#pragma once

#include "aspect/BaseAspectManager.h"
#include "concept/Concept.h"

#include <functional>
#include <map>
#include <memory>
#include <string>

namespace space {
class SPViewStackElement;

namespace feature { class FeatureManager; }
namespace search { class SearchManager; }

namespace concepts {

/// Registry of concepts. Registering a concept activates it with a recording
/// ConceptContext; unregistering deactivates it and removes everything it
/// contributed.
class ConceptManager : public aspect::BaseAspectManager<Concept> {
public:
    /// What a concept may contribute to.
    struct Host {
        SPViewStackElement* viewStack = nullptr;
        feature::FeatureManager* features = nullptr;
        search::SearchManager* search = nullptr;
        std::function<void()> refreshSidebar;
    };

    explicit ConceptManager(Host host);
    /// Deactivates every concept (in reverse order) without touching the UI,
    /// which is being torn down.
    ~ConceptManager() override;

    Concept* registerAspect(std::unique_ptr<Concept> item) override;
    void unregisterAspect(const std::string& id) override;

private:
    class RecordingContext;

    Host m_host;
    std::map<std::string, std::unique_ptr<RecordingContext>> m_contexts;
};

} // namespace concepts
} // namespace space
