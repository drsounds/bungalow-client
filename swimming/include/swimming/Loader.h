#ifndef SWIMMING_LOADER_H
#define SWIMMING_LOADER_H

#include <functional>
#include <string>

#include "swimming/Bridge.h"

/// Loads an app into a bridge from a script, an HTML page or an HTTP URL.
namespace swimming {
    /// Downloads url and returns the body; throws std::runtime_error on failure.
    using Fetch = std::function<std::string(const std::string& url)>;

    /// Runs source in bridge. source is an http(s) URL (downloaded with
    /// fetch) or a file path. HTML pages, recognised by extension or
    /// doctype, have their body elements added to the layout tree and their
    /// inline and classic external scripts run in document order; module
    /// scripts are skipped in favour of the Vite plugin's IIFE bundle.
    /// Anything else runs as a script. Throws std::runtime_error, or
    /// JsError when a script fails.
    void runSource(Bridge& bridge, const std::string& source, const Fetch& fetch = {});

    /// Resolves reference against the absolute URL base, like a browser.
    std::string resolveUrl(const std::string& base, const std::string& reference);
}

#endif
