---
layout: default
title: Aspects & Services
nav_order: 4
---

# Aspects & Services

This is the layer that replaces the monolith. Instead of one object that "is
Spotify", Bungalow has **Services** (back-ends with a lifecycle) that opt into
**aspects** (capability interfaces). Callers find back-ends *by capability*.

## The Service contract

[`Service`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/service/Service.java) is identity plus an
Android-style lifecycle:

```java
public interface Service extends Aspect {
    String getId();
    String getName();
    default ImageIcon getServiceIcon() { return null; }

    default void onCreate()  {}   // when registered
    default void onStart()   {}   // on ServiceManager.startAll()
    default void onStop()    {}   // on ServiceManager.stopAll()
    default void onDestroy() {}   // when permanently removed
}
```

`ServiceManager.register(s)` calls `onCreate()`; `startAll()` calls `onStart()`
on everything. Removing a Service calls `onStop()` then `onDestroy()`.

## Capabilities are aspects, not subclasses

A Service takes on a capability by **implementing an interface**. Because they're
interfaces, one Service object can combine several — a back-end could *discover*
and *stream* and *require auth* all at once.

| Aspect | Package | Responsibility | Replaces (in the monolith) |
|--------|---------|----------------|----------------------------|
| [`MediaService`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/plugin/media/service/MediaService.java) | `plugin.media.service` | play / pause / seek / load URI, playback events, optional player component | **streaming** |
| [`MusicService`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/plugin/music/service/MusicService.java) | `plugin.music.service` | `loadByIsrc`, `loadByTitleArtist`, and **non-loading** `lookup` / `lookupByTitleArtist` | **resolution / lookup** |
| [`MusicCatalogueService`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/plugin/catalogue/service/MusicCatalogueService.java) | `plugin.catalogue.service` | search + contextual browse (artist → releases → recordings) | **discovery / catalogue** |
| [`PlaylistService`](https://github.com/drsounds/bungalow/tree/main/src/main/java/se/spacify/service/playlist) | `service.playlist` | named playlists and their items | **library / playlists** |
| [`AuthAspect`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/service/AuthAspect.java) | `service` | `login` / `logout` / `isAuthenticated` / `getAccount` | **account / purchase gate** |

`MusicService extends MediaService` (a music streamer is also a media player);
`MusicCatalogueService extends Service` (discovery need not play anything).

## Selecting by capability

The whole point: callers never name a vendor. They ask the `ServiceManager` for
a capability and get every back-end that provides it.

```java
// "every back-end that can search a catalogue" — drives the Catalogs sidebar
for (MusicCatalogueService cat : sm.getServices(MusicCatalogueService.class)) { … }

// "every back-end that can play" — wired to the footer / Now Playing
for (MediaService ms : sm.getServices(MediaService.class)) { wireMediaService(ms); }

// "the first back-end that needs login" — only these show a sign-in button
AuthAspect auth = sm.getService(AuthAspect.class);
```

`getService(Class)` returns the first match; `getServices(Class)` returns all.
Both test `aspect.isInstance(service)`, so a Service is offered for *every*
interface it implements.

## How the un-bundling plays out

The five things a streaming monolith fuses together are five independent
contributions here:

- **Discovery** — a `MusicCatalogueService` (e.g. the MusicBrainz plugin). The
  `catalog` plugin builds a "Catalogs" sidebar folder from *all* registered
  catalogue services, so adding a Discogs plugin would add a Discogs catalogue
  with no core changes.
- **Resolution / lookup** — `MusicService.lookup(isrc)` lets the
  `PlaybackCoordinator` ask *every* streamer "can you play this ISRC?" and let the
  user choose. A track is not bound to one vendor; it is resolved at play time.
- **Streaming** — `MediaService`. Local files (`localmusic`) and YouTube
  (`youtube`) are just two implementations; the active one drives the transport.
- **Listening** — the vendor-neutral `PlayQueue` holds `PlayQueueItem`s whose
  play action runs through the coordinator. See
  [Playback & resolution](playback-and-resolution.html).
- **Purchase / account** — `AuthAspect` plus store front-ends rendered as web
  views (`web` plugin). Buying is just another Service capability + a View.

## Concepts and Features

Two coarser-grained contributions sit above raw Services:

- **[`Concept`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/concept/Concept.java)** — a feature
  *domain* a plugin activates (the library, search, playlists). It receives a
  [`ConceptContext`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/concept/ConceptContext.java)
  (the same registration surface as `PluginContext`) and registers the views and
  sidebar that make up that domain. Example:
  [`LibraryConcept`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/plugin/library/concept/LibraryConcept.java)
  registers the tracks/recordings/releases/artists views and the "Your Library"
  subtree, and keeps the Releases/Artists lists live via `LibraryEvents`.
- **[`Feature`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/feature/Feature.java)** — a simpler
  bundle that just exposes `getViews()` + `getSidebarNodes()`; `registerFeature`
  wires both automatically. Use a Feature for "a few screens + nav"; use a
  Concept when the domain needs activation/teardown logic and live state.

> Concept is the newest layer and the direction the codebase is migrating
> toward (plugins → register one Concept → the Concept contributes everything).
> Activation is wired: `registerConcept` calls `Concept.onActivate(ConceptContext)`
> and records the contributions for clean teardown — see
> [Status & roadmap](status-and-roadmap.html).
