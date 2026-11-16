#pragma once

#include "concept/Concept.h"

#include <shore/concept_manager.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace space {
class SPViewStackElement;

namespace feature { class FeatureManager; }
namespace search { class SearchManager; }

namespace concepts {

/// Shore's registry of concepts, wired to the client. Registering a concept
/// activates it with a recording ConceptContext; unregistering deactivates
/// it and removes everything it contributed.
class ConceptManager : public shore::SHConceptManager {
public:
    /// What a concept may contribute to.
    struct Host {
        SPViewStackElement* viewStack = nullptr;
        feature::FeatureManager* features = nullptr;
        search::SearchManager* search = nullptr;
        std::function<void()> refreshSidebar;
    };

    explicit ConceptManager(Host host);

    /// Takes ownership of the concept, activates it and returns it. A concept
    /// with the same id is unregistered first.
    Concept* registerAspect(std::unique_ptr<Concept> item);
    void unregisterAspect(const std::string& id) { Unregister(id); }
    Concept* get(const std::string& id) const { return dynamic_cast<Concept*>(Get(id)); }
    /// Every registered concept, in registration order.
    std::vector<Concept*> all() const;

protected:
    std::unique_ptr<shore::SHConceptContext> CreateContext(const std::string& conceptId) override;

private:
    class RecordingContext;

    Host m_host;
};

} // namespace concepts
} // namespace space
