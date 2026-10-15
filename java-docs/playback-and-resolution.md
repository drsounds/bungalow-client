---
layout: default
title: Playback & Resolution
nav_order: 8
---

# Playback & resolution

Listening is deliberately separated from *which vendor plays the bytes*. The
queue holds vendor-neutral entries; a track is **resolved to a Service at play
time**, and the user can pin a choice. This is the anti-lock-in heart of the app:
the same library row can play from local files today and a streaming service
tomorrow, with no change to the library.

Types: [`PlaybackCoordinator`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/service/media/PlaybackCoordinator.java),
`PlayQueue`, `PlayQueueItem`, `PlayRequest`, `ServiceMatch`, `ResolutionStore`,
[`ServiceMatchDialog`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/ui/ServiceMatchDialog.java),
and the `MusicServiceTrack` entity.

## PlayRequest — a vendor-neutral play intent

A view doesn't say "play this Spotify URI". It produces a `PlayRequest` carrying
whatever identity it has — `isrc`, `title`, `artist`, an optional `fallbackUri`,
`durationMs`, and (if it came from the local DB) the `Track`. Library views
override `playRequestAt(row)` to build one.

That identity is expressed portably as a [`musik:` URI](musik-uri-scheme.html): a
recording's play URI is `musik:isrc:<ISRC>`, and `MusikUri.toPlayRequest` /
`MusikUri.isrcOf` turn such a URI back into a `PlayRequest` (extracting the ISRC or
title/artist coordinates), so a stored playlist row or a dropped drag payload
resolves across Services exactly like a live library row. `PlaybackCoordinator.playUri`
routes a `musik:` URI through this same resolver.

## The PlayQueue

`PlayQueue` is the listening layer: an ordered list of `PlayQueueItem`s, a current
index, and change listeners. Each `PlayQueueItem` carries display metadata, a
stable **key** (used to highlight the now-playing row across views), and a
`playAction` Runnable — so the queue never references a Service directly:

```java
new PlayQueueItem(req.key(), title, artist, durationMs,
                  () -> PlaybackCoordinator.resolveAndPlay(req),   // the play action
                  req);                                            // the source request
```

`setQueueAndPlay(items, startIndex)` plays `items.get(startIndex).play()`, which
runs that action. When a track ends, `MediaService.onCompleted` advances the
queue (wired in `MainWindow.wireMediaService`).

## The PlaybackCoordinator

`PlaybackCoordinator` routes a `PlayRequest` across the installed Services. It is
wired to the window once at start-up via `PlaybackCoordinator.init(mainWindow)`
(its static entry points reach Services through that window).

`resolveAndPlay(req)`:

```
resolveAndPlay(req, forceChooser=false)
  ├─ if not forcing: ResolutionStore.find(req)         // a saved "Play with…" pick?
  │     └─ play it on that Service and return
  └─ gatherAndChoose(req)                               // otherwise resolve fresh
        ├─ (off the EDT) gatherMatches(req):
        │     for each MusicService:
        │       lookup(isrc)  ?? lookupByTitleArtist(title, artist)  → ServiceMatch
        ├─ 0 matches → play fallbackUri, or "nothing can play this"
        └─ ≥1 match  → ServiceMatchDialog → user picks a Service
              ├─ play it (that Service becomes the *active* Service)
              └─ if "always": ResolutionStore.save(req, match)
```

`gatherMatches` does blocking network lookups, so it runs on a `SwingWorker`
background thread; the dialog and playback happen back on the EDT.

## "Play with…" — pinning a resolution

This is the *open-as* model for music. The first time an unresolved track plays,
the `ServiceMatchDialog` shows every Service that matched (icon + name + detail)
and a **"Always play this track with this match"** checkbox. Ticking it persists a
`MusicServiceTrack` row (`ResolutionStore.save`) keyed by the track (by FK if it's
a local `Track`, else by a stable ISRC/URI key). Next time, the saved pick plays
straight away — no dialog.

You can re-open the chooser at any time via the row's right-click **"Play with…"**
(`resolveAndPlay(req, true)` forces the chooser even when a pick is saved),
re-running the lookup and letting you change services.

## The active Service

Whichever Service handled the most recent play is the **active** Service
(`getActiveService()`). Listeners (`addActiveServiceListener`) fire on change, so
the footer transport and the Now Playing panel switch to that Service's player
component and target — e.g. swapping in YouTube's embedded JCEF player when a
track resolves to YouTube, and back to the local audio surface otherwise.

> Wiring note: `PlaybackCoordinator.init` **must** run at start-up. If
> `staticMainWindow` is null, `gatherMatches` can't reach the Services, comes back
> empty, and the resolution dialog never appears.
