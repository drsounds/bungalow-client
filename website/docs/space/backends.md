---
title: UI backends
sidebar_position: 6
---

# UI backends

Space doesn't draw or receive input itself. A **backend** supplies a native
window, forwards input events to the element tree and implements
`SPGraphicsContext` for drawing. The backend is chosen at configure time with
`BUNGALOW_UI_BACKEND`; see [Building](../getting-started/building.md).

## `SPGraphicsContext`

Everything Space draws goes through this interface:

| Group | Methods |
|-------|---------|
| Shapes | `drawLine`, `drawRectangle`, `fillRectangle`, `fillRoundedRectangle`, `drawRoundedRectangle`, `fillRoundedLinearGradientRectangle`, `drawLinearGradient` |
| Images | `loadImage`, `drawImage`, `drawImagePart`, `drawImageSlices` (nine-patch), `drawSkinImage` |
| Text | `drawString`, `drawHTMLText`, `measureString`, `setFontStyle` |
| Clipping and origin | `setClip`, `declipRect`, `setOrigo`, `restoreOrigo` |
| Native controls | `drawControl`, `drawInputElement`, `setControlAttribute` |
| Repaint | `invalidateRegion`, `freeze`, `thaw`, `themeChanged` |
| Network | `request(method, url, body, callback)` |

The base class does nothing, so a backend only implements what it supports.

## Backends

| Backend | Window and drawing | Embedded web | Entry point |
|---------|--------------------|--------------|-------------|
| **wxWidgets** | `SpaceFrame` / `SpaceWidget`, `WxWidgetsGraphicsContext` (`wxwidgets/src`) | wxWebView (WebView2 on Windows, WebKit on macOS) | `wxwidgets/app/src/main.cpp` |
| **WTL** | `Win32WindowElement`, `Win32GraphicsContext` (GDI+) | CEF (with `BUNGALOW_ENABLE_CEF`) | `windows/src/wtl_main.cpp` |
| **Win32** | `Win32WindowElement`, `Win32GraphicsContext` | none | `windows/src/main.cpp` |

The **WTL** backend is the main Windows client. It shares the app views in
`wxwidgets/app/src` (`AppView`, `LibraryView`, `LocalFilesView`,
`PlayQueueView`, `SPAppView`) with the wxWidgets backend and adds Windows
implementations of the platform services:

| Service | File |
|---------|------|
| `BungalowApp` (implements `SPApplicationContext`) | `windows/src/BungalowApp.cpp` |
| audio playback | `windows/src/WinAudio.cpp` |
| HTTP | `windows/src/WinHttpClient.cpp` (WinHTTP) |
| downloads | `windows/src/DownloadManager.cpp` |
| favicons | `windows/src/FaviconCache.cpp` |
| UI-thread dispatch | `windows/src/UiDispatcher.cpp` |
| known folders, dialogs | `windows/src/PlatformUi.cpp` (`space::platform`) |
| web sites | `windows/src/SiteView.cpp` |
| visualiser | `SPPlayerCanvasElement`, `SPMusicVisualizerElement` |

The **Win32** backend compiles only Space and a minimal window. It is useful
for testing the framework on its own.

## Platform helpers

Each backend provides `space::platform` functions that the client uses for
paths:

| Function | Windows |
|----------|---------|
| `userDataDir()` | `%APPDATA%\Bungalow` (created if missing) |
| `homeDir()` | the user profile folder |
| `documentsDir()` | the Documents folder |

## Embedded browsers

`space/include/SPEmbeddedBrowser.h` declares two backend-neutral browser
interfaces. Each backend provides the factory functions:

```cpp
std::unique_ptr<SPEmbeddedBrowser> createEmbeddedAppBrowser(
    const SPAppBundle& bundle, SPElement* host,
    SPEmbeddedBrowser::Options options, SPEmbeddedBrowser::Callbacks callbacks);

std::unique_ptr<SPEmbeddedSiteBrowser> createEmbeddedSiteBrowser(
    SPElement* host, SPEmbeddedSiteBrowser::Options options,
    SPEmbeddedSiteBrowser::Callbacks callbacks);
```

They return `nullptr` when no browser is available. All calls and callbacks
happen on the UI thread. See [HTML5 apps](../apps/html5-apps.md) and
[Sites and bookmarks](../apps/sites-and-bookmarks.md).

### CEF on WTL

`web::CefRuntime` (`windows/include/web/CefRuntime.h`) owns the process-wide
CEF instance, like `se.bungalow.web.CefRuntime` does for JCEF in the Java
client:

1. `executeSubprocess(instance)` runs first in `wWinMain`. If CEF started this
   process as a renderer or GPU subprocess, it returns the exit code.
2. `initialize(instance)` starts CEF in the browser process with its own
   message-loop thread. If it returns false, app bundles show an error view.
3. `shutdown()` runs after the main window has been destroyed. It waits for
   every browser to close.

The implementation is in `windows/src/web/cef` (namespace `space::web::cef`):
`CefAppBrowser`, `CefSiteBrowser`, `CefBrowserWindow` and `SpSchemeHandler`
(which serves `sp://`).

## Linux

`linux/` holds an older copy of the client and framework (with `MainElement`
instead of `ui::MainWindow`, and a top-level `StartFeature`) for Linux and
macOS builds. It is being merged into the shared `space/`, `bungalow/` and
`wxwidgets/` trees. Until that is finished, the wxWidgets backend is the
supported route on those platforms.
