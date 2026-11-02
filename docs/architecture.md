---
layout: default
title: Architecture Overview
nav_order: 2
---

# Architecture overview

The C++ client follows the package structure of the Java client
(`java/src/main/java/se/bungalow`). Each package is a directory under
`bungalow/include` and `bungalow/src` and a namespace under `space`:

| Package | Namespace | Java counterpart | Contents |
|---------|-----------|------------------|----------|
| `aspect` | `space::aspect` | `se.bungalow.aspect` | `Aspect`, `AspectManager<T>`, `BaseAspectManager<T>`: an insertion-ordered, owning id → aspect registry. |
| `feature` | `space::feature` | `se.bungalow.feature` | `Feature` (views + sidebar nodes) and `FeatureManager`, which mounts a feature's views in the view stack. |
| `concept` | `space::concepts` | `se.bungalow.concept` | `Concept`, `ConceptContext`, `ConceptManager`. The namespace is plural because `concept` is a C++20 keyword. |
| `search` | `space::search` | `se.bungalow.search` | `EntityKind`, `SearchResult`, `SearchProvider`, `SearchManager`. |
| `navigation` | `space::navigation` | `se.bungalow.navigation` | `SidebarNode`, the toolkit-independent sidebar model. |
| `ui` | `space::ui` | `se.bungalow.ui` | `MainWindow` (the shell, formerly `MainElement`) and `LeftLibraryMenu` (the sidebar). |
| `web` | `space::web` | `se.bungalow.web` | `SiteUri`, `BookmarkManager`, `BookmarkEvents`; `CefRuntime` lives in `windows/include/web` (see below). |
| `app` | `space::app` | `se.bungalow.app.*` | The built-in features and concepts, one directory per domain. |

## Startup

`ui::MainWindow::layout()` builds the shell from `views/app.xml`, mounts the
catch-all `SpaceView`, creates one manager per contribution kind and calls
`app::registerBuiltins()`:

```
StartFeature       MAIN      Start, Now Playing          bungalow:start, bungalow:player
SettingsFeature    LIBRARY   Settings                    bungalow:config
LibraryConcept     LIBRARY   Library › Tracks, Local     bungalow:library, bungalow:localfiles
                             + LibrarySearchProvider
DownloadsFeature   LIBRARY   Downloads                   bungalow:downloads
DatabaseFeature    DATABASE  Table Manager › tables      bungalow:database, bungalow:table:…
WebConcept         APPS, SITES (bookmarks)               bungalow:site:, app:, curator:, store:
SearchConcept      –                                     bungalow:search:<query>
```

Registration order matters twice over:

* The view stack routes a URI to the **last** view that accepts it, so the
  catch-all `SpaceView` is mounted before every contribution.
* Sidebar sections appear in the order they are first contributed. Sections
  with the same label (`LIBRARY` above) are merged.

## Features and concepts

A **feature** returns views from `createViews()` and sidebar nodes from
`getSidebarNodes()`. The sidebar is rebuilt from every feature's
`getSidebarNodes()` whenever its content changes, so a feature with live
content (custom tables, bookmarks) simply returns its current state.

A **concept** is a domain with a lifecycle. `onActivate()` receives a
`ConceptContext` and contributes features, views and search providers through
it. The context records each contribution, so unregistering the concept
removes exactly what it added. `onDeactivate()` releases anything acquired
outside the context, such as the web concept's `BookmarkEvents` listener.

Space elements are never freed (other elements may still hold listeners on
them), so unregistering a feature or concept detaches and hides its views
rather than deleting them.

## Search

`bungalow:search:<query>` (or `bungalow:search?q=<query>`) opens
`app::SearchView`, which runs the query through `SearchManager` and lists the
results ordered by `EntityKind`. Providers run on the UI thread. Double-clicking
a result plays it if `SPApplicationContext::canPlayUri()` accepts it (for
example a library track, `bungalow:track:<id>`), and navigates to it otherwise.

## Web and CEF

`web::SiteUri` translates `bungalow:site:` URIs to https URLs and back; query
strings and fragments pass through unchanged. `SiteView` uses it for its
URI ↔ URL mapping.

`web::CefRuntime` (`windows/include/web/CefRuntime.h`) owns the process-wide
Chromium Embedded Framework on the WTL backend, as `se.bungalow.web.CefRuntime`
does for JCEF: `executeSubprocess()` and `initialize()` at startup, and
`shutdown()` after the main window is gone. Its implementation and the app and
site browsers are in `windows/src/web/cef` (namespace `space::web::cef`), built
as the separate C++20 `bungalow-cef` library.
