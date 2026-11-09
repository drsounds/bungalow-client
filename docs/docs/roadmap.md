---
title: Status and roadmap
sidebar_position: 100
---

# Status and roadmap

The C++ client is being refactored to follow the Java client's package
structure. This page lists what is implemented, how it compares with the Java
client, and what comes next.

## What works today

- **The aspect model**: `BaseAspectManager<T>` with `FeatureManager`,
  `ConceptManager` and `SearchManager`. Concepts are activated with a recording
  context, and everything they add is removed when they are unregistered.
- **Built-ins as features and concepts**: Start, Settings, Library, Downloads,
  Database, Web and Search, registered by `app::registerBuiltins()`.
- **Navigation**: the view stack, `bungalow:` URIs, history, and a sidebar
  built from feature nodes with merged sections.
- **Unified search** across search providers, grouped by entity kind.
- **The Space framework**: XUL, Lua templates with postbacks, the URI store with
  a disk cache, and skins (modern, classic, Frutiger Aero, `skin.xml` bundles).
- **App bundles**: native Lua apps with routing and a sandbox, HTML5 apps in
  CEF with isolated storage and host consent, and the `sp://import` JS API.
- **Web**: sites, stores and curators in an embedded browser, hierarchical
  bookmarks, favicons and downloads.
- **Library**: a SQLite library keyed by ISRC, UPC and ISNI, local file import
  and user tables.
- **Backends**: wxWidgets, Win32 and WTL (with CEF), with CI and release
  workflows for Windows.

## Compared with the Java client

| Java client | C++ client |
|-------------|------------|
| `Aspect`, `BaseAspectManager` | ✅ `space::aspect` |
| `Feature`, `FeatureManager` | ✅ `space::feature` |
| `Concept`, `ConceptContext`, `ConceptManager` | ✅ `space::concepts` |
| `SidebarNode` | ✅ `space::navigation` (sections are merged; there is no `SidebarHandle`, features return their current nodes instead) |
| `SearchProvider`, `SearchManager` | ✅ `space::search` |
| `View`, `ViewStack` | ✅ `SPViewElement`, `SPViewStackElement` (last match wins rather than first) |
| Spider templates (LuaJ) | ✅ `SPTemplate`, `SPNativeApp` |
| `SiteUri`, bookmarks, `CefRuntime` | ✅ `space::web`, `windows/include/web` |
| Skins, themes (`Taste`, `ThemeManager`) | ✅ `SPSkin` with HSL tint; no separate Chrome/Design layer yet |
| `Plugin`, `PluginManager`, jar plugins | ⏳ app bundles cover apps and skins; there are no compiled plugins |
| `Service`, `ServiceManager`, capability lookup | ✅ `space::service` (`getServices<T>()`) |
| `MusicService`, `MusicCatalogueService`, `AuthAspect` | ✅ `space::service`; local files are the only installed back-end, and no catalogue or sign-in service exists yet |
| `PlaybackCoordinator`, "Play with…", `ResolutionStore` | ✅ `space::service`; the chooser is a popup menu and is skipped when only one service matches |
| `AvailabilityResolver` (Buy/Stream column) | ⏳ not yet: rows without a local file stay greyed out, though "Play with…" can still resolve them |
| `PlaylistService` | ⏳ playlists are stored by `RootlistManager` and the database, not by a service |
| `musik:` URIs (`MusikUri`, [RFC-0002](./specs/rfc-0002-musik-uri-scheme.md)) | ⏳ `musik:` is an alias of `bungalow:`; the identifiers are in the schema, but there is no RFC-0002 parser yet |
| Playlist sharing (`Upsl`, `Base62`, [RFC-0001](./specs/rfc-0001-universal-playlist-sharing.md)) | ⏳ not yet |
| Chrome / Design | ⏳ one shell (`views/app.xml`) |

## Known gaps

1. **Legacy views outside the packages.** Many views, data sources and models
   are still at the top level of `bungalow/`. They are registered through
   features but haven't been moved into `app/<domain>`.
2. **The `linux/` tree** is an older copy of the client and framework. It is
   being merged into the shared trees.
3. **The `spider/` prototype** is replaced by `SPTemplate` and `SPNativeApp`
   and can be removed.
4. **Template routing** in `SPElement::request` only knows the `start` and
   `artist` routes. Other templates need a lookup mechanism.
5. **Shared API data** in `sp://import` comes from a fictional catalogue until
   the C++ bridge is connected.

## Roadmap

1. **More services**: streaming and catalogue back-ends (for example
   MusicBrainz, Subsonic/Jellyfin) implementing the `space::service`
   capabilities, plus availability in track lists so rows without a local
   file aren't greyed out when another service can play them.
2. **Background resolution**: `PlaybackCoordinator` looks tracks up on the UI
   thread, which is fine for the library database but not for network
   services.
3. **`musik:` URIs** ([RFC-0002](./specs/rfc-0002-musik-uri-scheme.md)):
   parse the identifier, name and query forms instead of only aliasing the
   prefix, and use them in playlists, drag and drop and the queue.
4. **Decentralised playlists** ([RFC-0001](./specs/rfc-0001-universal-playlist-sharing.md)):
   create, parse and preview `musik:federated:` links, then add hosted
   links, Ed25519 verification and ActivityPub subscriptions.
5. **Catalogue search and a store aspect**: feed every `MusicCatalogueService`
   into unified search, and add a store capability so stores can be installed
   side by side.
6. **Connect the Apps API** to the library, player and search.
7. **Declarative `<app>` concepts**, so a whole feature (entities, views,
   sidebar) can be declared in markup or YAML, as described in the
   [introduction](./intro.md#the-space-ui-idea).
8. **JavaScript views**: write views in JavaScript on an embedded V8 engine,
   so integrations can extend Bungalow without C++ or Lua:

   ```javascript
   import render from '@bungalow/ui'

   function Page() {
      return render('view', { flex: 1 },
         render('text', {}, 'Hello World!'))
   }
   ```

9. **Chrome and Design**: switchable shells (for example a Windows Media
   Player-style shell) in addition to skins.

## Contributor conventions

- Everything pluggable implements `aspect::Aspect` and is registered with a
  manager.
- Concepts contribute **only** through their `ConceptContext`, so removing
  them is clean. Anything acquired outside it is released in `onDeactivate()`.
- Views are reached by `bungalow:` URI, never by direct reference.
- Feature sidebar nodes are computed from current state. Call
  `refreshSidebar()` when that state changes.
- Comments explain *why* and match the surrounding code's level of detail.
