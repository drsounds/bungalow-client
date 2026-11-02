---
title: Project layout
sidebar_position: 2
---

# Project layout

```
bungalow-client/
├── space/          Space UI framework: elements, XUL parser, templates, skins, URI store, app bundles
│   ├── include/    SP*.h public headers
│   ├── src/
│   └── tests/
├── bungalow/       The Bungalow client, built on Space
│   ├── include/    one directory per package (aspect, feature, concept, …) + legacy views/models
│   ├── src/
│   └── tests/
├── windows/        Win32 and WTL backends, Windows platform services, CEF host (src/web/cef)
├── wxwidgets/      wxWidgets backend (src/) and the views shared with WTL (app/)
├── linux/          Older copy of the client and framework (Linux/macOS), being merged back
├── spider/         Early Spider request/view prototype (superseded by SPTemplate)
├── elua/           Embedded-Lua template preprocessor (C)
├── views/          Shell and built-in XUL: app.xml, start.xml, artist.xml, error.xml
├── Apps/           Bundled app bundles: import (JS API), native, modern (skin), testapp
├── icons/          Sidebar SVG icons
├── resources/      bungalow.db (optional bundled database)
├── ports/cef/      vcpkg overlay port for CEF
├── java/           The Java reference client and its docs (java/docs)
├── docs/           Short earlier notes, now covered by this site
└── website/        This documentation site (Docusaurus)
```

## Libraries and targets

| Target | Sources | Notes |
|--------|---------|-------|
| `elua` | `elua/src` | static C library; turns template text into Lua source |
| `bungalow-cef` | `windows/src/web/cef` | C++20 static library, only with `BUNGALOW_ENABLE_CEF` |
| `bungalow-client` | `space/src` + `bungalow/src` + the selected backend | the application |

Which `bungalow/src` files are compiled depends on the backend. The
`wxwidgets` and `wtl` builds include the package directories
(`aspect app concept feature navigation search ui web`). The minimal `win32`
build compiles only Space and the Win32 backend.

## Namespaces

| Namespace | Where |
|-----------|-------|
| `space` | the framework (`SPElement`, `SPViewStackElement`, `SPSkin`, …) |
| `space::aspect`, `space::feature`, `space::concepts`, `space::search`, `space::navigation`, `space::ui`, `space::web`, `space::app` | the client packages; see [architecture](../architecture/overview.md) |
| `space::views` | concrete views (`LibraryView`, `SiteView`, `AppView`, …) |
| `space::web::cef` | the CEF host |
| `space::platform` | per-backend platform helpers (`userDataDir()`, …) |
