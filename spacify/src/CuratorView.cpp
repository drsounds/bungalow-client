#include "CuratorView.h"
#include "SPString.h"

namespace space {
namespace views {

CuratorView::CuratorView(SPElement* parent)
    : SiteView(parent, "spacify:curator:", true, true) {}

bool CuratorView::acceptsUri(const string& uri) {
    return space::startsWith(uri, "spacify:curator:");
}

// spacify:curator:mycurator       -> https://mycurator.curator.spacify.se
// spacify:curator:example.com     -> https://example.com
// spacify:curator:mycurator:page  -> https://mycurator.curator.spacify.se/page
string CuratorView::uriToUrl(const string& uri) const {
    const string prefix = "spacify:curator:";
    if (uri.size() <= prefix.size()) return "";

    string remainder = uri.substr(prefix.size());
    size_t colon     = remainder.find(':');
    string domain    = (colon == string::npos) ? remainder : remainder.substr(0, colon);
    string rest      = (colon == string::npos) ? "" : remainder.substr(colon);

    if (domain.find('.') == string::npos && !domain.empty())
        domain += kCuratorDomain;

    string url = "https://" + domain;
    for (char c : rest)
        url += (c == ':') ? '/' : c;
    return url;
}

// https://mycurator.curator.spacify.se/page -> spacify:curator:mycurator:page
// https://example.com/page                  -> spacify:curator:example.com:page
string CuratorView::urlToUri(const string& url) const {
    const string scheme = "https://";
    if (url.rfind(scheme, 0) != 0) return "";

    string rest   = url.substr(scheme.size());
    size_t slash  = rest.find('/');
    string domain = (slash == string::npos) ? rest : rest.substr(0, slash);
    string path   = (slash == string::npos) ? "" : rest.substr(slash);

    const string suffix = kCuratorDomain;
    if (domain.size() > suffix.size() &&
        domain.substr(domain.size() - suffix.size()) == suffix)
        domain = domain.substr(0, domain.size() - suffix.size());

    string uri = "spacify:curator:" + domain;
    for (char c : path)
        uri += (c == '/') ? ':' : c;
    while (!uri.empty() && uri.back() == ':')
        uri.pop_back();
    return uri;
}

} // namespace views
} // namespace space
