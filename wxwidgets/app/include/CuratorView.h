#pragma once
#include <SiteView.h>

namespace space {
namespace views {

// Handles spacify:curator: URIs — bare names expand to <name>.curator.spacify.se
class CuratorView : public SiteView {
public:
    explicit CuratorView(SPElement* parent);
    bool   acceptsUri(const string& uri) override;
    string uriToUrl(const string& uri)   const override;
    string urlToUri(const string& url)   const override;

private:
    static constexpr const char* kCuratorDomain = ".curator.spacify.se";
};

} // namespace views
} // namespace space
