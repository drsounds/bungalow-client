#include "app/BuiltinRegistry.h"

#include "app/database/DatabaseFeature.h"
#include "app/downloads/DownloadsFeature.h"
#include "app/library/LibraryConcept.h"
#include "app/playlist/PlaylistConcept.h"
#include "app/search/SearchConcept.h"
#include "app/settings/SettingsFeature.h"
#include "app/start/StartFeature.h"
#include "app/web/WebConcept.h"

#include "concept/ConceptManager.h"
#include "feature/FeatureManager.h"

namespace space {
namespace app {

void registerBuiltins(feature::FeatureManager& features, concepts::ConceptManager& concepts) {
    features.registerAspect(std::make_unique<StartFeature>());
    features.registerAspect(std::make_unique<SettingsFeature>());
    concepts.registerAspect(std::make_unique<LibraryConcept>());
    features.registerAspect(std::make_unique<DownloadsFeature>());
    features.registerAspect(std::make_unique<DatabaseFeature>());
    concepts.registerAspect(std::make_unique<WebConcept>());
    concepts.registerAspect(std::make_unique<SearchConcept>());
    // Last, so the rootlist comes after every other sidebar entry.
    concepts.registerAspect(std::make_unique<PlaylistConcept>());
}

} // namespace app
} // namespace space
