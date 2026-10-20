#include "StoreView.h"
#include "SPString.h"

namespace space {
namespace views {

StoreView::StoreView(SPElement* parent)
    : SiteView(parent, "bungalow:store:", true, true) {}

bool StoreView::acceptsUri(const string& uri) {
    return space::startsWith(uri, "bungalow:store:");
}

// bungalow:store:mystore       -> https://mystore.store.bungalow.se
// bungalow:store:example.com   -> https://example.com
// bungalow:store:mystore:page  -> https://mystore.store.bungalow.se/page
string StoreView::uriToUrl(const string& uri) const {
    const string prefix = "bungalow:store:";
    if (uri.size() <= prefix.size()) return "";

    string remainder = uri.substr(prefix.size());
    size_t colon     = remainder.find(':');
    string domain    = (colon == string::npos) ? remainder : remainder.substr(0, colon);
    string rest      = (colon == string::npos) ? "" : remainder.substr(colon);

    if (domain.find('.') == string::npos && !domain.empty())
        domain += kStoreDomain;

    string url = "https://" + domain;
    for (char c : rest)
        url += (c == ':') ? '/' : c;
    return url;
}

// https://mystore.store.bungalow.se/page -> bungalow:store:mystore:page
// https://example.com/page              -> bungalow:store:example.com:page
string StoreView::urlToUri(const string& url) const {
    const string scheme = "https://";
    if (url.rfind(scheme, 0) != 0) return "";

    string rest   = url.substr(scheme.size());
    size_t slash  = rest.find('/');
    string domain = (slash == string::npos) ? rest : rest.substr(0, slash);
    string path   = (slash == string::npos) ? "" : rest.substr(slash);

    const string suffix = kStoreDomain;
    if (domain.size() > suffix.size() &&
        domain.substr(domain.size() - suffix.size()) == suffix)
        domain = domain.substr(0, domain.size() - suffix.size());

    string uri = "bungalow:store:" + domain;
    for (char c : path)
        uri += (c == '/') ? ':' : c;
    while (!uri.empty() && uri.back() == ':')
        uri.pop_back();
    return uri;
}

} // namespace views
} // namespace space
