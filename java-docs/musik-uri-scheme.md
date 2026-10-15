---
layout: default
title: "The musik: URI Scheme"
nav_order: 9
---

# The `musik:` URI scheme

`musik:` is Bungalow's **portable, service-agnostic content identifier** — the way
the app *names* a recording, work, release, artist, genre or mood without tying it
to any one provider's row ids. Because a `musik:` URI carries identity rather than
a vendor target, the same URI resolves on whatever Services a given install has.
It is the atom that playlists, library rows, drag-and-drop payloads and the play
queue are built from.

The wire format is specified formally in
[RFC-0002](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/music/spec/RFC-0002-musik-uri-scheme.md).
This page is the practical overview: the forms, and how the app resolves them.

## The forms

**Identifier forms** — a standard, registered id (preferred, most portable):

| URI | Names |
|---|---|
| `musik:isrc:<ISRC>` | a recording (ISRC — the canonical recording key) |
| `musik:iswc:<ISWC>` | a work / composition |
| `musik:upc:<UPC>` | a release |
| `musik:isni:<ISNI>` | an artist / creator |
| `musik:ipi:<IPI>` | a creator by IPI name number |
| `musik:genre:<slug>` | a genre facet (e.g. `musik:genre:drum-bass`) |
| `musik:mood:<slug>` | a mood facet (e.g. `musik:mood:melancholic`) |

**Name form** — human-readable coordinates, used when no id is known. Every free
text segment is percent-encoded:

```
musik:artist:<artist>[:release:<release>[[:track:<0-99>]:name:<name>[:version:<version>]]]
```

```
musik:artist:Aphex%20Twin
musik:artist:Aphex%20Twin:release:Drukqs
musik:artist:Aphex%20Twin:release:Drukqs:track:1:name:Jynweythek
```

**Query form** — a flat equivalent of the name form, naming a recording:

```
musik:track?name=<name>&artist_name=<artist>&release_name=<release>&number=<0-99>&version=<version>
```

## How the app resolves it

The parser/builder/resolver is
[`MusikUri`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/music/net/MusikUri.java) — the
music-app analogue of [`SiteUri`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/web/SiteUri.java):

- **`MusikUri.parse(uri)`** → a typed value (`kind` + coordinates), or `null`.
- **`new MusikUri(...)` factories + `toString()`** → build & round-trip a URI.
- **`MusikUri.toPlayRequest(uri)`** → a vendor-neutral
  [`PlayRequest`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/service/media/PlayRequest.java) for the
  *playable* forms (an `isrc` recording, or a name/query-form recording), so
  playback flows through the normal
  [`PlaybackCoordinator`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/service/media/PlaybackCoordinator.java)
  path — by ISRC first, then by title/artist metadata (see
  [Playback & resolution](playback-and-resolution.html)).
- **`MusikUri.isrcOf(uri)`** → the ISRC of a recording URI, or `null`.

Where it plugs in:

- [`Recording.getPlayUri()`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/music/model/Recording.java)
  now emits `musik:isrc:<ISRC>` — the canonical content URI stored in playlist
  rows and carried in drag-and-drop payloads.
- [`PlayableKind.fromUri`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/music/model/PlayableKind.java)
  recognises `musik:` schemes (`musik:upc:` → a release, otherwise a track), so a
  dragged row lands in a playlist tagged with the right kind.
- [`PlaybackCoordinator.playUri`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/service/media/PlaybackCoordinator.java)
  and the playlist view resolve `musik:` identity through `MusikUri`.

Only the **playable** forms are wired into playback today. `iswc`, `upc`, `isni`,
`ipi`, `genre` and `mood` parse into typed references but are not yet browsable —
they name entities the library UI will grow pages/filters for later.

## Relationship to `spacify:` URIs

`spacify:` URIs (`spacify:library`, `spacify:playlist:<id>`, `spacify:release:…`)
address **app views/navigation**, not portable content, and are unchanged. The one
legacy *content* URI, `spacify:recording:isrc:<ISRC>`, is the predecessor of
`musik:isrc:<ISRC>`; the app still accepts it and treats it as equivalent, so
playlists and saved data written before this change keep working.
