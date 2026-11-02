#include "web/SiteUri.h"

#include <iostream>
#include <string>

using space::web::SiteUri;

namespace {

int failures = 0;

void expect(const std::string& actual, const std::string& expected, const char* what) {
    if (actual == expected) return;
    std::cerr << what << ": expected \"" << expected << "\", got \"" << actual << "\"\n";
    ++failures;
}

void expect(bool actual, bool expected, const char* what) {
    expect(std::string(actual ? "true" : "false"), expected ? "true" : "false", what);
}

} // namespace

int main() {
    expect(SiteUri::toUrl("bungalow:site:example.com:foo:bar?x=1"), "https://example.com/foo/bar?x=1", "toUrl path+query");
    expect(SiteUri::toUrl("bungalow:site:example.com"), "https://example.com", "toUrl root");
    expect(SiteUri::toUrl("bungalow:site:example.com:a::b:"), "https://example.com/a/b", "toUrl empty segments");
    expect(SiteUri::toUrl("bungalow:site:example.com:s?q=a:b#c:d"), "https://example.com/s?q=a:b#c:d", "toUrl keeps colons in query");
    expect(SiteUri::toUrl("bungalow:site:"), "", "toUrl without host");
    expect(SiteUri::toUrl("bungalow:site::foo"), "", "toUrl empty host");
    expect(SiteUri::toUrl("bungalow:app:x"), "", "toUrl other prefix");
    expect(SiteUri::toUrl("bungalow:store:shop.example:a", SiteUri::kStorePrefix), "https://shop.example/a", "toUrl store prefix");

    expect(SiteUri::toBungalowUri("https://example.com/foo/bar?x=1"), "bungalow:site:example.com:foo:bar?x=1", "toBungalowUri path+query");
    expect(SiteUri::toBungalowUri("https://example.com/"), "bungalow:site:example.com", "toBungalowUri trailing slash");
    expect(SiteUri::toBungalowUri("https://example.com"), "bungalow:site:example.com", "toBungalowUri root");
    expect(SiteUri::toBungalowUri("https://example.com/s?q=a/b#x/y?z"), "bungalow:site:example.com:s?q=a/b#x/y?z", "toBungalowUri keeps slashes in query");
    expect(SiteUri::toBungalowUri("  https://user@example.com:8443/a  "), "bungalow:site:example.com:a", "toBungalowUri drops user and port");
    expect(SiteUri::toBungalowUri("https://[::1]:8080/a"), "bungalow:site:[::1]:a", "toBungalowUri IPv6 host");
    expect(SiteUri::toBungalowUri("example.com/a"), "", "toBungalowUri without scheme");
    expect(SiteUri::toBungalowUri("https:///a"), "", "toBungalowUri without host");

    const std::string url = "https://example.com/a/b?q=1:2#frag";
    expect(SiteUri::toUrl(SiteUri::toBungalowUri(url)), url, "round trip");

    expect(SiteUri::host("bungalow:site:example.com:a?b"), "example.com", "host");
    expect(SiteUri::host("bungalow:site:example.com?b"), "example.com", "host before query");
    expect(SiteUri::isRoot("bungalow:site:example.com"), true, "isRoot root");
    expect(SiteUri::isRoot("bungalow:site:example.com?x=1"), true, "isRoot query only");
    expect(SiteUri::isRoot("bungalow:site:example.com:a"), false, "isRoot sub-page");
    expect(SiteUri::rootUri("bungalow:site:example.com:a:b"), "bungalow:site:example.com", "rootUri");
    expect(SiteUri::isSiteUri("bungalow:store:x"), false, "isSiteUri other prefix");

    return failures == 0 ? 0 : 1;
}
