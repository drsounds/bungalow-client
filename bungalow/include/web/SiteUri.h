#pragma once

#include <string>

namespace space {
namespace web {

/// Translates between bungalow:site: navigation URIs and https:// URLs.
///
/// Grammar: bungalow:site:<host>[:<segment>...][?query][#fragment].
/// Colon-separated segments after the host become path components; the query
/// and fragment pass through unchanged, colons and slashes included.
///
///   bungalow:site:example.com:foo:bar?x=1  <->  https://example.com/foo/bar?x=1
///   bungalow:site:example.com              <->  https://example.com
///
/// Every function takes the URI prefix, so the same grammar serves
/// bungalow:store: and other site-like schemes. Functions return an empty
/// string where there is no valid result.
class SiteUri {
public:
    static constexpr const char* kPrefix = "bungalow:site:";
    static constexpr const char* kStorePrefix = "bungalow:store:";

    SiteUri() = delete;

    static bool isSiteUri(const std::string& uri) { return hasPrefix(uri, kPrefix); }
    static bool hasPrefix(const std::string& uri, const std::string& prefix);

    /// The host of a site URI (first segment after the prefix).
    static std::string host(const std::string& uri, const std::string& prefix = kPrefix);
    /// True if the URI points at a host root (no path segments).
    static bool isRoot(const std::string& uri, const std::string& prefix = kPrefix);
    /// The <prefix><host> root URI for any site URI.
    static std::string rootUri(const std::string& uri, const std::string& prefix = kPrefix);

    /// <prefix>… -> https://…
    static std::string toUrl(const std::string& uri, const std::string& prefix = kPrefix);
    /// scheme://host/… -> <prefix>host:… The port and user info are dropped.
    static std::string toBungalowUri(const std::string& url, const std::string& prefix = kPrefix);
};

} // namespace web
} // namespace space
