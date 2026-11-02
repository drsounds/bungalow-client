#pragma once

#include <Windows.h>

// Process-wide Chromium Embedded Framework lifetime for the WTL backend.
// Free of CEF headers so the C++17 client can include it.
namespace space {
namespace cefhost {

// Runs this process as a CEF subprocess (renderer, GPU, ...) when CEF launched
// it as one. Returns the exit code the process must exit with, or -1 for the
// browser process. Call first thing in wWinMain.
int executeSubprocess(HINSTANCE instance);

// Starts CEF in the browser process with its own message-loop thread. Returns
// false if CEF is unavailable; app bundles then show an error view.
bool initialize(HINSTANCE instance);
bool isInitialized();

// Waits for every browser to close, then shuts CEF down. Call once on the
// thread that called initialize(), after the main window has been destroyed
// (which tears down the browser windows parented to it).
void shutdown();

} // namespace cefhost
} // namespace space
