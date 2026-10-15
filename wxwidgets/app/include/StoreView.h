#pragma once
#include <SiteView.h>

namespace space {
namespace views {

// Handles spacify:store: URIs — bare names expand to <name>.store.spacify.se
class StoreView : public SiteView {
public:
    explicit StoreView(SPElement* parent);
    bool   acceptsUri(const string& uri) override;
    string uriToUrl(const string& uri)   const override;
    string urlToUri(const string& url)   const override;

private:
    static constexpr const char* kStoreDomain = ".store.spacify.se";
};

} // namespace views
} // namespace space
