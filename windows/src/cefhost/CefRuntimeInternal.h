#pragma once

#include <string>

#include "include/cef_browser.h"

// Shared between the CEF runtime and the app browsers.
namespace space {
namespace cefhost {

// Scheme serving app bundles: sp://<bundle identifier>/<path>.
inline constexpr char kAppScheme[] = "sp";

// Browser -> renderer: extra_info key holding the app's startup script.
inline constexpr char kStartupScriptKey[] = "startupScript";

// Renderer -> browser: window.spacify.navigate(uri). Argument 0 is the URI.
inline constexpr char kNavigateMessage[] = "spacify.navigate";

// Root of all CEF profile data; each app gets a subdirectory.
std::wstring cacheRoot();

// Bookkeeping so closeAllBrowsers() can wait for every browser to go away.
// Returns false once shutdown has begun; the browser must not be created.
bool beginBrowserCreation();
void abortBrowserCreation();
void browserCreated(CefRefPtr<CefBrowser> browser);
void browserClosed(CefRefPtr<CefBrowser> browser);

// Force-closes every browser and waits (up to 5 s) until they are gone,
// pumping the calling thread's messages meanwhile.
void closeAllBrowsers();

} // namespace cefhost
} // namespace space
