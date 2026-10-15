---
layout: default
title: Architecture Overview
nav_order: 2
---

# Architecture overview

## The layers

Bungalow is organised as a thin application shell around a registry of pluggable
contributions. Nothing in the "content" layers knows about a specific vendor;
they only know **aspects** (capability interfaces).

```
┌───────────────────────────────────────────────────────────────────────┐
│  app            SpacifyApp.main → MainWindow (JFrame, the "OS shell")   │
├───────────────────────────────────────────────────────────────────────┤
│  managers       one AspectManager per contribution kind:               │
│                 ServiceManager · ConceptManager · FeatureManager        │
│                 ChromeManager · DesignManager · SkinManager · ThemeMgr  │
├───────────────────────────────────────────────────────────────────────┤
│  plugin system  PluginManager · PluginLoader · PluginContext (Recorder) │
│                 discovers, activates, and *undoes* plugin contributions │
├───────────────────────────────────────────────────────────────────────┤
│  contributions  Service · Concept · Feature · View · SidebarNode        │
│                 Entity · Chrome · Skin · Design · Theme                  │
├───────────────────────────────────────────────────────────────────────┤
│  aspects        MediaService · MusicService · MusicCatalogueService     │
│                 PlaylistService · AuthAspect  (capability interfaces)   │
├───────────────────────────────────────────────────────────────────────┤
│  infra          DatabaseManager (ORMLite/SQLite connection + DAO registry)│
│                 web (JCEF) · controls                                    │
└───────────────────────────────────────────────────────────────────────┘
```

### Why one `AspectManager` per kind

Every pluggable thing implements [`Aspect`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/aspect/Aspect.java):

```java
public interface Aspect {
    String getId();
    String getName();
    void onRegister(AspectManager<? extends Aspect> aspectManager);
}
```

A [`BaseAspectManager<T>`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/aspect/BaseAspectManager.java)
is just an insertion-ordered `id → T` registry with `register / unregister /
get / all`. Each concrete manager (`ServiceManager`, `SkinManager`, …) extends it
and adds typed lookups — most importantly **selection by capability**:

```java
serviceManager.getServices(MusicCatalogueService.class);  // every Service that can search
serviceManager.getServices(MediaService.class);           // every Service that can play
```

This is the core of the un-bundling: a caller asks for *a capability*, not *a
vendor*. See [Aspects & Services](aspects-and-services.html).

## Boot sequence

`SpacifyApp.main` ([app/SpacifyApp.java](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/SpacifyApp.java))
sets the Nimbus look-and-feel, calls `DatabaseManager.getInstance().init()`,
installs a shutdown hook (dispose JCEF, close the DB), then on the EDT does
`new MainWindow()` and shows it.

The whole system comes up inside the `MainWindow` constructor
([ui/MainWindow.java](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/ui/MainWindow.java)), in a
deliberate order — the ordering is load-bearing, because plugins register *into*
the managers, so the managers (and the shared sidebar) must exist first:

```
1.  instance = this                         // so detached panels can resolve theme/skin
2.  create ViewStack, Taste, and every AspectManager
3.  create the shared LeftLibraryMenu       // sidebar is owned here, not by the Chrome
4.  PluginManager.init(...).start()         // plugins register Services/Designs/Skins/…
5.  ConfigManager.load()                    // apply persisted theme + accent + design id
6.  ServiceManager.startAll()               // drive Service.onStart()
7.  PlaybackCoordinator.init(this)          // wire static play/resolve entry points
8.  setDesign(pickInitialDesign())          // build + install the Chrome (footer, splits, sidebar)
9.  wire every MediaService to footer/now-playing + auto-advance the queue
10. navigate("spacify:now-playing")
```

> **Why the order matters (a real bug this prevented):** if step 4 ran before
> step 2, a plugin's `onActivate` would call `getServiceManager().register(...)`
> on a `null` manager. That start-up cycle (plugin → manager) is exactly what the
> ordering above resolves.

## Runtime flow: playing a track

A double-click in a library view is the clearest end-to-end path through the
layers:

```
View (e.g. TracksLibraryView)
  └─ playRequestAt(row) → PlayRequest{ isrc, title, artist, fallbackUri, … }
       └─ PlayQueue.setQueueAndPlay(...)        // listening / queue layer
            └─ PlaybackCoordinator.resolveAndPlay(req)
                 ├─ ResolutionStore.find(req)   // saved "Play with…" pick? play it
                 └─ gatherMatches(req)           // ask every MusicService (off the EDT)
                      └─ ServiceMatchDialog      // user picks a Service for this track
                           └─ MediaService.play()  → becomes the *active* Service
                                └─ footer + Now Playing follow the active Service
```

No layer names a vendor. The view produces a vendor-neutral `PlayRequest`; the
coordinator resolves it across whatever `MusicService`s happen to be installed;
the chosen back-end becomes active and the transport UI follows it. Content itself
is named vendor-neutrally too, by a [`musik:` URI](musik-uri-scheme.html) (e.g.
`musik:isrc:<ISRC>`) — the portable identifier stored in playlists and carried in
drag payloads. Full detail in [Playback & resolution](playback-and-resolution.html).

## Where state lives

- **Entity data** — SQLite via ORMLite. Entities are owned **per app/concept**,
  not globally: each concept declares its entity classes in its own `model`
  package and registers their tables on activation via
  `ConceptContext.registerEntity(Class)` (e.g. the `music` concept owns the shared
  content model — `Artist`/`Recording`/`MusicRelease`/`Track`/… in
  [`app/music/model`](https://github.com/drsounds/bungalow/tree/main/src/main/java/se/spacify/app/music/model/) — while
  `playlist`, `downloads`, `web`, `media` and `localmusic` each own their own
  tables). [`DatabaseManager`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/db/DatabaseManager.java)
  knows about no concrete entity; it just owns the connection and hands out cached,
  table-creating DAOs via `dao(Class)`. See
  [`db/PLANS.md`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/db/PLANS.md).
- **Plugin enable/disable** — `~/.spacify/plugins/state.properties`.
- **Per-plugin settings** — `~/.spacify/plugins/<id>.properties` via `PluginSettings`.
- **Theme / tint / accent / selected design** — `~/.spacify/settings.properties`
  via [`ConfigManager`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/config/ConfigManager.java).
