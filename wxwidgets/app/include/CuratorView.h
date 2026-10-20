#pragma once
#include <SiteView.h>

namespace space {
namespace views {

// Handles bungalow:curator: URIs — bare names expand to <name>.curator.bungalow.se
class CuratorView : public SiteView {
public:
    explicit CuratorView(SPElement* parent);
    bool   acceptsUri(const string& uri) override;
    string uriToUrl(const string& uri)   const override;
    string urlToUri(const string& url)   const override;

private:
    static constexpr const char* kCuratorDomain = ".curator.bungalow.se";
};

} // namespace views
} // namespace space
