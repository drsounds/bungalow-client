---
title: Architecture overview
sidebar_position: 1
---

# Architecture overview

Bungalow is a thin application shell around registries of pluggable
contributions. It runs on Space, a platform-neutral UI framework, which runs on
one of several native backends.

```
┌───────────────────────────────────────────────────────────────────────────┐
│  backend      BungalowApp (wxWidgets / WTL) · window · graphics context     │
│               platform services: audio, HTTP, downloads, favicons, CEF      │
├───────────────────────────────────────────────────────────────────────────┤
│  shell        ui::MainWindow: header, sidebar (LeftLibraryMenu), view stack,│
│               play queue, footer — built from views/app.xml                 │
├───────────────────────────────────────────────────────────────────────────┤
│  managers     FeatureManager · ConceptManager · SearchManager               │
│               (each a BaseAspectManager<T>: owning, insertion-ordered)      │
├───────────────────────────────────────────────────────────────────────────┤
│  contributions app::registerBuiltins(): StartFeature, SettingsFeature,      │
│               LibraryConcept, DownloadsFeature, DatabaseFeature,            │
│               WebConcept, SearchConcept                                     │
├───────────────────────────────────────────────────────────────────────────┤
│  apps         SPAppManager: app bundles in Apps/ and ~/Bungalow             │
│               native Lua apps (SPNativeApp) · HTML5 apps (CEF) · skins      │
├───────────────────────────────────────────────────────────────────────────┤
│  Space        SPElement tree · XUL · SPTemplate (Lua) · SPUriStore ·        │
│               SPSkin · SPViewStackElement                                   │
├───────────────────────────────────────────────────────────────────────────┤
│  data         Database (SQLite): library, downloads, bookmarks, user tables │
└───────────────────────────────────────────────────────────────────────────┘
```

## Packages

The C++ client follows the package structure of the
[Java client](../java-client/architecture.md) (`java/src/main/java/se/bungalow`).
Each package is a directory under `bungalow/include` and `bungalow/src`, and a
namespace under `space`:

| Package | Namespace | Java counterpart | Contents |
|---------|-----------|------------------|----------|
| `aspect` | `space::aspect` | `se.bungalow.aspect` | `Aspect`, `AspectManager<T>`, `BaseAspectManager<T>`: an insertion-ordered, owning id → aspect registry. See [Aspects](./aspects.md). |
| `feature` | `space::feature` | `se.bungalow.feature` | `Feature` (views and sidebar nodes) and `FeatureManager`, which mounts a feature's views in the view stack. |
| `concept` | `space::concepts` | `se.bungalow.concept` | `Concept`, `ConceptContext`, `ConceptManager`. The namespace is plural because `concept` is a C++20 keyword. |
| `search` | `space::search` | `se.bungalow.search` | `EntityKind`, `SearchResult`, `SearchProvider`, `SearchManager`. See [Search](./search.md). |
| `navigation` | `space::navigation` | `se.bungalow.navigation` | `SidebarNode`, the toolkit-independent sidebar model. |
| `ui` | `space::ui` | `se.bungalow.ui` | `MainWindow` (the shell, formerly `MainElement`) and `LeftLibraryMenu` (the sidebar). |
| `web` | `space::web` | `se.bungalow.web` | `SiteUri`, `BookmarkManager`, `BookmarkEvents`; `CefRuntime` is in `windows/include/web`. |
| `app` | `space::app` | `se.bungalow.app.*` | The built-in features and concepts, one directory per domain, plus `AppContext` and `BuiltinRegistry`. |

Older code that predates the packages is still at the top level of
`bungalow/include` and `bungalow/src`: the views (`LibraryView`, `ConfigView`,
`DownloadsView`, `CustomTableView`, …), the data sources, the `SP*` models in
`models/` and the services in `services/`.

## Startup

The backend's `BungalowApp` prepares the process and then creates the window:

```
1. scan app bundles               SPAppManager::addDefaultSearchPaths() + scan()
                                  (before the skin, which may come from a bundle)
2. open the database              resources/bungalow.db, else <user data>/bungalow.db
3. create the download manager
4. configure the URI store        JSON cache in <user data>/cache/uris,
                                  registerEntityResolvers() for artists and tracks
5. new ui::MainWindow             → loadXUL("views/app.xml") → layout()
6. load settings.json             skin, colour scheme, tint, button style, developer mode
7. create and apply the skin      frutiger-aero | classic | modern | a skin bundle
```

`ui::MainWindow::layout()` builds the shell, then the contribution system:

```
1. find the header, sidebar tree, view stack and footer elements from app.xml
2. mount the catch-all SpaceView in the view stack
3. create FeatureManager(viewStack), SearchManager and
   ConceptManager(Host{viewStack, features, search, rebuildNavItems})
4. app::registerBuiltins(features, concepts)
5. rebuildNavItems()                  → sidebar from every feature's nodes
6. add the play-queue panel and footer
7. navigate("bungalow:start")
```

The managers are created before anything registers into them. This is the same
rule the Java client's boot sequence follows.

## Built-in contributions

`app::registerBuiltins()` registers, in this order:

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

Registration order matters in two ways:

* The view stack routes a URI to the **last** view that accepts it, so the
  catch-all `SpaceView` is mounted before every contribution. See
  [Navigation](./navigation.md).
* Sidebar sections appear in the order they are first contributed. Sections
  with the same label (`LIBRARY` above) are merged.

## Teardown

`MainWindow` declares its managers so that **concepts are deactivated first**:
`ConceptManager`'s destructor calls `onDeactivate()` on every concept in
reverse order without touching the UI. Space elements are never freed while
the window is alive, because other elements may still hold listeners on them.
Unregistering a feature or concept therefore detaches and hides its views
instead of deleting them.

On the WTL backend, `web::CefRuntime::shutdown()` runs after the main window
has been destroyed.

## Where state lives

| State | Location |
|-------|----------|
| Library, downloads, bookmarks, accepted domains, user tables | SQLite: `resources/bungalow.db` if present, else `<user data>/bungalow.db` |
| Settings (skin, tint, colour scheme, developer mode, …) | `<user data>/settings.json` |
| URI entity cache | `<user data>/cache/uris/` |
| Favicons | `<user data>/favicons/` |
| CEF profile and cache | `<user data>/cef/` |

`<user data>` is `%APPDATA%\Bungalow` on Windows. See
[Settings and files](../reference/settings-and-files.md).
