#pragma once

#include "concept/Concept.h"

#include "FaviconCache.h"

#include <memory>

namespace space {
namespace app {

/// Web browsing: the site, app, curator and store views, and the APPS and
/// SITES sidebar sections listing their bookmarks. Follows BookmarkEvents to
/// keep those sections and their favicons current.
class WebConcept : public concepts::Concept {
public:
    WebConcept();
    ~WebConcept() override;

    std::string getId() const override { return "web"; }
    std::string getName() const override { return "Web Browser"; }
    std::string getIconPath() const override { return "icons/Web.svg"; }
    void onActivate(concepts::ConceptContext& ctx) override;
    void onDeactivate() override;

private:
    std::unique_ptr<FaviconCache> m_favicons;
    concepts::ConceptContext* m_ctx = nullptr;
    int m_bookmarkListener = 0;
};

} // namespace app
} // namespace space
