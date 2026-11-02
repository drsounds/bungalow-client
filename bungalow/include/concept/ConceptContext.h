#pragma once

#include "SPViewStackElement.h"

#include <memory>
#include <string>

namespace space {
class SPViewElement;

namespace feature { class Feature; }
namespace search {
class SearchManager;
class SearchProvider;
}

namespace concepts {

/// Registration surface handed to Concept::onActivate(). Every contribution
/// made through it is recorded, so unregistering the concept undoes exactly
/// what it added.
class ConceptContext {
public:
    virtual ~ConceptContext() = default;

    /// Id of the concept being activated.
    virtual std::string conceptId() const = 0;

    /// Registers a feature; its views and sidebar nodes are wired automatically.
    virtual feature::Feature* registerFeature(std::unique_ptr<feature::Feature> feature) = 0;
    /// Mounts a view in the bungalow: URI space. Create it with viewStack()
    /// as its parent.
    virtual void registerView(SPViewElement* view) = 0;
    /// Registers a search provider; its results appear in the search view.
    virtual search::SearchProvider* registerSearchProvider(std::unique_ptr<search::SearchProvider> provider) = 0;

    /// The app's view stack, for views that drive navigation.
    virtual SPViewStackElement* viewStack() = 0;
    virtual search::SearchManager& searchManager() = 0;
    /// Rebuilds the sidebar from every feature's current nodes. Call after
    /// live sidebar content (bookmarks, …) changed.
    virtual void refreshSidebar() = 0;
};

} // namespace concepts
} // namespace space
