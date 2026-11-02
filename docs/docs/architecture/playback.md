---
title: Playback
sidebar_position: 6
---

# Playback

Playback is separated from the views that start it. A view builds a list of
display entries. The application turns them into a play queue and picks the
**media service** that can play each entry's URI.

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
    std::string uri;          // resolved playback URI (empty when !playable)
    bool        playable = true; // false → greyed out, skipped by next/prev
};

struct PlayContext {
    std::vector<PlayQueueEntry> entries;
    std::size_t                 startIndex = 0;
};
```

`BungalowApp::playContext()` replaces the queue and plays the start entry.
`playbackNext()` and `playbackPrev()` skip entries that aren't playable. The
queue is shown in the `PlayQueueView` panel next to the view stack.

`playUri(uri)` is the single-item shortcut:

* `bungalow:track:<id>` plays a track from the library database.
* Any other URI is played if a media service accepts it.

`canPlayUri()` returns true for exactly those URIs. Search results and
templates use it to decide between playing and navigating.

## Media services

```cpp
class SPService {
public:
    virtual bool acceptsUri(const std::string& uri) const = 0;
    virtual void startLogin() = 0;
    virtual void logout() = 0;
    virtual std::string getId() const = 0;
    virtual std::string getName() const = 0;
    virtual bool isLoggedIn() const = 0;
    virtual std::string getLoggedInUser() const = 0;
};

class SPMediaService : public SPService {
public:
    virtual void load(const std::string& uri) = 0;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void seek(int posMs) = 0;
    virtual int getDurationMs() const { return 0; }
    virtual int getPositionMs() const { return 0; }
};
```

The application chooses the first service whose `acceptsUri()` returns true.
`LocalMediaService` (id `local`) accepts `file://` URIs and plays MP3, AIFF,
M4A and WAV. It uses `AVAudioPlayer` on macOS; on Windows, playback goes
through `windows/src/WinAudio.cpp`.

## Compared with the Java client

The Java client resolves a vendor-neutral `PlayRequest` across every installed
`MusicService` at play time, lets you choose with a "Play with…" dialog and
remembers the choice. The C++ client doesn't do cross-service resolution yet; a
URI is matched to a single service. See
[Playback & resolution](../java-client/playback-and-resolution.md) and the
[roadmap](../roadmap.md).
