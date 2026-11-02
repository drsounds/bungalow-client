#include "app/search/SearchConcept.h"

#include "app/search/SearchView.h"
#include "concept/ConceptContext.h"

namespace space {
namespace app {

void SearchConcept::onActivate(concepts::ConceptContext& ctx) {
    ctx.registerView(new SearchView(ctx.viewStack(), ctx.searchManager()));
}

} // namespace app
} // namespace space
