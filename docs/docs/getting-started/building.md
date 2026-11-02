---
title: Building
sidebar_position: 1
---

# Building

Bungalow is built with **CMake** (3.16 or later) and gets its dependencies from
**vcpkg** in manifest mode ([`vcpkg.json`](https://github.com/drsounds/bungalow-client/blob/main/vcpkg.json)).
The client is C++17. The optional CEF host library is C++20.

## Dependencies

| Library | Used for |
|---------|----------|
| `nlohmann-json` | manifests, settings, the URI store, template data |
| `lua` | Lua templates and native apps |
| `libpng` | image loading |
| `expat` (2.2.8 or later) | the XUL / XML parser |
| `sqlite3` | the library database (Win32/WTL backends) |
| `wtl` | the WTL backend |
| `wxwidgets` with `webview` | the wxWidgets backend (vcpkg feature `wxwidgets`) |
| `cef` | HTML5 apps and web sites on WTL (vcpkg feature `cef`, from the overlay port in `ports/cef`) |

On Windows, the wxWidgets WebView backend also needs the Microsoft Edge
WebView2 Runtime.

## Choosing a UI backend

Space draws through a platform backend. Choose it with `BUNGALOW_UI_BACKEND`:

| Value | Platforms | Notes |
|-------|-----------|-------|
| `wxwidgets` (default) | Windows, macOS, Linux | wxWidgets frame plus wxWebView for sites |
| `win32` | Windows | the plain Win32 / GDI+ backend; a minimal shell |
| `wtl` | Windows | the full Windows client; needs the WTL headers (`-DWTL_INCLUDE_DIR=<path>`) |

See [UI backends](../space/backends.md) for what each backend implements.

## Building with the presets (Windows, WTL + CEF)

[`CMakePresets.json`](https://github.com/drsounds/bungalow-client/blob/main/CMakePresets.json)
defines two Ninja presets. Both use the WTL backend and expect `VCPKG_ROOT` to
point at a vcpkg checkout:

| Preset | Build directory | CEF |
|--------|-----------------|-----|
| `ninja` | `build/ninja` | on |
| `ninja-nocef` | `build/ninja-nocef` | off |

From a Visual Studio developer prompt:

```powershell
cmake --preset ninja
cmake --build --preset ninja
ctest --preset ninja
```

Use `ninja-nocef` if you don't need HTML5 apps. It builds faster and doesn't
download the CEF binary distribution.

## Building with CEF

`-DBUNGALOW_ENABLE_CEF=ON` (on in the `ninja` preset) runs local HTML5 app
bundles and web sites in the Chromium Embedded Framework. It needs the WTL
backend.

CEF comes from the overlay port in `ports/cef`, which vcpkg installs into
`vcpkg_installed/`: the prebuilt CEF binary distribution plus a locally built
`libcef_dll_wrapper`. The CEF headers need C++20, so the CEF host code
(`windows/src/web/cef`) is built as a separate `bungalow-cef` static library,
and the client only sees the CEF-free `SPEmbeddedBrowser.h` and `CefRuntime.h`
headers. The CEF runtime files are copied next to the executable after each
build.

## Building with wxWidgets

```bash
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_MANIFEST_FEATURES=wxwidgets
cmake --build build
```

## Tests

`include(CTest)` registers these test executables. Run them with
`ctest --test-dir <build dir>`:

| Test | Covers |
|------|--------|
| `bungalow-template-test` | `SPTemplate`: Lua templates, `${…}` and `%` lines |
| `bungalow-native-app-test` | `SPNativeApp`: routes, controllers, sandbox, instruction limit |
| `bungalow-uri-store-test` | `SPUriStore`: reducer, resolvers, persistence |
| `bungalow-string-test` | string helpers |
| `bungalow-xml-parser-test` | the Expat-based XUL parser |
| `bungalow-site-uri-test` | `web::SiteUri` translation |
| `bungalow-aspect-test` | `BaseAspectManager` registration and replacement |

## Continuous integration

`.github/workflows` contains a **Native Windows CI** workflow (WTL backend,
configure, build and test) and release workflows for the `win32`, `wtl` and
`wxwidgets` backends. They are currently started by hand (`workflow_dispatch`)
from the Actions tab.
