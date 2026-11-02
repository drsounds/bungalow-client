#include "web/SiteUri.h"

#include <algorithm>
#include <cctype>

namespace space {
namespace web {
namespace {

std::string trim(const std::string& s) {
    size_t begin = 0;
    size_t end = s.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(s[begin]))) ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(begin, end - begin);
}

} // namespace

bool SiteUri::hasPrefix(const std::string& uri, const std::string& prefix) {
    return uri.rfind(prefix, 0) == 0;
}

std::string SiteUri::host(const std::string& uri, const std::string& prefix) {
    if (!hasPrefix(uri, prefix)) return {};
    const std::string rest = uri.substr(prefix.size());
    return rest.substr(0, rest.find_first_of(":?#"));
}

bool SiteUri::isRoot(const std::string& uri, const std::string& prefix) {
    if (!hasPrefix(uri, prefix)) return false;
    const std::string rest = uri.substr(prefix.size());
    for (char c : rest) {
        if (c == ':') return false;            // has a path segment
        if (c == '?' || c == '#') return true; // query/fragment only
    }
    return true;
}

std::string SiteUri::rootUri(const std::string& uri, const std::string& prefix) {
    const std::string h = host(uri, prefix);
    return h.empty() ? std::string() : prefix + h;
}

std::string SiteUri::toUrl(const std::string& uri, const std::string& prefix) {
    if (!hasPrefix(uri, prefix)) return {};
    const std::string rest = uri.substr(prefix.size());

    // Split off the ?query / #fragment suffix (whichever comes first).
    const size_t cut = rest.find_first_of("?#");
    const std::string pathPart = rest.substr(0, cut);
    const std::string suffix = cut == std::string::npos ? std::string() : rest.substr(cut);
    if (pathPart.empty() || pathPart.front() == ':') return {};

    std::string url = "https://";
    size_t start = 0;
    bool first = true;
    while (start <= pathPart.size()) {
        size_t end = pathPart.find(':', start);
        if (end == std::string::npos) end = pathPart.size();
        const std::string segment = pathPart.substr(start, end - start);
        if (first) url += segment;
        else if (!segment.empty()) url += '/' + segment;
        first = false;
        start = end + 1;
    }
    return url + suffix;
}

std::string SiteUri::toBungalowUri(const std::string& url, const std::string& prefix) {
    const std::string u = trim(url);
    const size_t schemeEnd = u.find("://");
    if (schemeEnd == std::string::npos || schemeEnd == 0) return {};

    const size_t authorityStart = schemeEnd + 3;
    const size_t authorityEnd = std::min(u.find_first_of("/?#", authorityStart), u.size());
    std::string host = u.substr(authorityStart, authorityEnd - authorityStart);
    const size_t at = host.rfind('@');
    if (at != std::string::npos) host = host.substr(at + 1);
    const size_t port = host.rfind(':');
    if (port != std::string::npos && host.find(']', port) == std::string::npos) host = host.substr(0, port);
    if (host.empty()) return {};

    // The fragment starts at the first '#'; a '?' after it is part of it.
    const size_t hash = u.find('#', authorityEnd);
    const std::string beforeFragment = u.substr(authorityEnd, hash == std::string::npos ? std::string::npos : hash - authorityEnd);
    const size_t question = beforeFragment.find('?');
    const std::string path = beforeFragment.substr(0, question);

    std::string result = prefix + host;
    size_t start = 0;
    while (start < path.size()) {
        size_t end = path.find('/', start);
        if (end == std::string::npos) end = path.size();
        if (end > start) result += ':' + path.substr(start, end - start);
        start = end + 1;
    }
    if (question != std::string::npos) result += beforeFragment.substr(question);
    if (hash != std::string::npos) result += u.substr(hash);
    return result;
}

} // namespace web
} // namespace space
