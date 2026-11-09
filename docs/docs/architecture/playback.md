---
title: Playback
sidebar_position: 6
---

# Playback

Playback is separated from the views that start it, and from the vendor that
plays the bytes. A view builds a list of entries carrying whatever identity it
has (a file, an ISRC, a library track). The application turns them into a play
queue, and each entry is **resolved to a service at play time**, so the same
library row can play from local files today and a streaming service tomorrow.

## `SPApplicationContext`

Space-level elements (the footer, the now-playing bar, the seek bar, the search
view) reach playback through an interface in `space/include/SPApplicationContext.h`.
The backend's `BungalowApp` implements it and installs itself with
`SPApplicationContext::setCurrent(this)`:

```cpp
class SPApplicationContext {
public:
    virtual SPObservable& getPlaybackEvents() = 0;
    virtual bool hasCurrentPlayable() const = 0;
    virtual std::string currentPlayableUri() const = 0;
    virtual const std::string& getTrackTitle() const = 0;
    virtual const std::string& getArtistName() const = 0;
    virtual void seekTo(int positionMs) = 0;
    virtual void togglePlayPause() = 0;
    virtual void playbackPrev() = 0;
    virtual void playbackNext() = 0;
    virtual bool canPlayUri(const std::string& uri) const { return false; }
    virtual void playUri(const std::string& uri) {}

    static SPApplicationContext* current();
    static void setCurrent(SPApplicationContext* context);
};
```

Elements subscribe to `getPlaybackEvents()` to follow track and position
changes.

## Play contexts and the queue

A view starts playback by passing a `PlayContext`, which is the whole list in
display order plus the index to start from:

```cpp
struct PlayQueueEntry {
    std::string displayTitle;
    std::string displayArtist;
    std::string coverArtPath;
    std::string uri;             // direct playback URI, e.g. the local file (may be empty)
    bool        playable = true; // false → greyed out, skipped by next/prev
    std::string isrc;            // identity resolved across music services
    int64_t     trackId = 0;     // library track, 0 when not from the library
};

struct PlayContext {
    std::vector<PlayQueueEntry> entries;
    std::size_t                 startIndex = 0;
    bool                        chooseService = false; // "Play with…"
};
```

`BungalowApp::playContext()` replaces the queue and plays the start entry. An
entry stays playable when a service accepts its `uri` or it has an ISRC.
`playbackNext()` and `playbackPrev()` skip entries that aren't playable. The
queue is shown in the `PlayQueueView` panel next to the view stack.

`playUri(uri)` is the single-item shortcut:

* `bungalow:track:<id>` plays a track from the library database.
* `musik:isrc:<ISRC>` names a recording and is resolved across the music
  services.
* Any other URI is played if a media service accepts it.

`canPlayUri()` returns true for exactly those URIs. Search results and
templates use it to decide between playing and navigating.

## Services

The back-ends are `space::service` Services, selected by capability (see
[Aspects](./aspects.md#services-and-capabilities)):

* `MediaService`: `acceptsUri()`, `loadUri()`, `play()`, `pause()`, `stop()`,
  `seek()`, position and duration, and playback listeners. A failed load is
  reported with `fireError()`, which moves the service to the `Error` state.
* `MusicService`: a `MediaService` that can also `lookup(isrc)` and
  `lookupByTitleArtist(title, artist)` without loading anything, and offers
  `findCandidates()` for the chooser.

`LocalMediaService` (id `local`) is a `MusicService`. It accepts `file://`
URIs and plays MP3, AIFF, M4A and WAV through the `sp_audio_*` bridge
(`windows/src/WinAudio.cpp` on Windows, `AVAudioPlayerBridge.mm` on macOS).
Its lookups find library tracks of that recording or title that have a local
file.

## Resolution

`BungalowApp` turns the entry it is about to play into a `PlayRequest`
(`trackId`, `isrc`, title, artist, and the entry's `uri` as `fallbackUri`) and
hands it to the `PlaybackCoordinator`:

```
resolveAndPlay(request, forceChooser)
  ├─ unless forced: a remembered pick (ResolutionStore)? → play it on that service
  ├─ gatherMatches(request): for each MusicService
  │     accepts fallbackUri? → that exact item
  │     else findCandidates(isrc, title, artist)
  ├─ no match  → play fallbackUri on a service that accepts it, else NotFound
  ├─ one match → play it (unless forced)
  └─ several   → the chooser; "Always play with…" saves the pick
```

The service that played becomes the **active** service
(`getActiveService()`); pause, seek and the position timer talk to it.

The chooser is a popup menu listing each match twice: "Play with …" and
"Always play with …". The track context menu in the library views has a
**Play with…** item, which forces the chooser even when one service matches
or a pick is remembered, and lets a track without a local file try the other
services.

Picks are stored in the `music_service_tracks` table by `ResolutionStore`.
A pick is bound to the library track (deleted with it) and to the request's
`trackKey()` (`isrc:<ISRC>`, else the URI, else title and artist), and
records the matched URI so it replays the exact item.

Lookups run on the UI thread. That's fine for the library database; network
services will need resolution moved to a background thread.

`bungalow/tests/ServiceTest.cpp` covers capability lookup, the service
lifecycle, resolution and remembered picks.

## Compared with the Java client

This follows the Java client's
[Playback & resolution](../java-client/playback-and-resolution.md), with two
differences: the chooser is skipped when exactly one service matches, and
there is no `AvailabilityResolver` yet, so track lists still grey out rows
without a local file. See the [roadmap](../roadmap.md).
