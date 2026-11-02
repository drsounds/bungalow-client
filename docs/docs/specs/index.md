---
title: Specifications
sidebar_label: Overview
sidebar_position: 1
slug: /specs/
---

# Specifications

Two RFCs define how Bungalow **names music** and how it **shares playlists**
without a central service. Both are wire formats only: they define what a URI
looks like and how to read it. How an app shows or plays the result is left to
each implementation.

| RFC | Status | Defines |
|-----|--------|---------|
| [RFC-0001: Universal Playlist Sharing Link (UPSL)](./rfc-0001-universal-playlist-sharing.md) | Accepted, v1 frozen | `bungalow:federated:` playlist links, federation, signing |
| [RFC-0002: The `musik:` URI scheme](./rfc-0002-musik-uri-scheme.md) | Provisional | service-independent names for recordings, works, releases, artists, genres and moods |

RFC-0002 is the building block: a UPSL playlist is an ordered list of `musik:`
URIs.

## `musik:`: naming music without a vendor

A `musik:` URI names a piece of music by a **standard identifier** when one is
known, and by **human-readable coordinates** when not:

```
musik:isrc:GBAYE0601498                                     a recording
musik:upc:00602557091936                                    a release
musik:isni:000000012146438X                                 an artist
musik:artist:Aphex%20Twin:release:Drukqs:track:1:name:Jynweythek
musik:track?name=Jynweythek&artist_name=Aphex%20Twin        the same, as a query
musik:genre:drum-bass                                       a genre facet
```

No service is named in the URI. Each reader looks the item up on the services
it has, so the same URI plays from local files on one machine and from a
streaming service on another. Producers should prefer `isrc` and fall back to
the name form with as many coordinates as they know.

## UPSL: decentralised playlists

A **Universal Playlist Sharing Link** puts a whole playlist into one URI:

```
bungalow:federated:user:@drsounds@mastodon.social:playlist:<uuid>
    :<name>:description:<description>[:image:<cover>]:uris:<ref>,<ref>,…[:sig:<signature>]
```

* **No central catalogue.** Small playlists are embedded entirely in the link.
  Name, description and every content reference are Base62-encoded, so user
  text can never be mistaken for a delimiter.
* **Federated identity.** Each playlist belongs to a Mastodon-style
  `@user@instance` handle and has a random UUID that stays the same across
  edits.
* **Optional hub.** Without a body, the link is a *hosted* reference: the
  issuer's instance serves the current contents. That also lets recipients
  subscribe and get ActivityPub `Update`s when the playlist changes.
  Subscriptions and pingbacks are guidelines, and save and play pingbacks
  require the user's consent.
* **Verifiable origin.** A link can be signed with the instance's Ed25519 key,
  found through WebFinger. An unsigned link is shown as "shared by *handle*"
  and isn't marked verified.
* **An HTTPS mirror** at `https://ups.bungalow.net/user/<handle>/playlist/<uuid>/…`
  (the domain is provisional) lets links travel where custom schemes don't.
* **Size limit.** Embedded links must stay within 2048 characters. Larger
  playlists use the hosted form.

## Implementation status

| | Java client | C++ client |
|---|---|---|
| `musik:` parsing (`MusikUri`) | ✅ identifier, name and query forms; `isrc` and name forms are playable | ⏳ not parsed. `MainWindow` treats `musik:` (like `space:`, `spotify:` and `spacify:`) as an alias of `bungalow:` when navigating, and site and app browsers hand `musik:` links to the client |
| Content keys in the library | ✅ `Recording.getPlayUri()` emits `musik:isrc:` | ✅ the schema is keyed by ISRC, UPC and ISNI |
| Legacy `bungalow:recording:isrc:` | ✅ accepted as equivalent | n/a |
| UPSL parsing (`Upsl`, `Base62`) | ✅ handle, playlist id, name and description; also accepts the `acct:`, `musik:acct:` and `bungalow:acct:` prefixes | ⏳ not yet |
| UPSL preview (`PlaylistController`) | ✅ renders a preview of a shared playlist | ⏳ not yet |
| Content refs, signatures, hosted form, federation | ⏳ not yet | ⏳ not yet |

For the Java client's resolution model, see
[The `musik:` URI scheme](../java-client/musik-uri-scheme.md) and
[Playback & resolution](../java-client/playback-and-resolution.md). The C++
plan is in the [roadmap](../roadmap.md).

The original informal sketch that RFC-0001 grew out of is kept next to the Java
sources as
[`UNIVERSAL_PLAYLIST_SHARING.md`](https://github.com/drsounds/bungalow-client/blob/main/java/src/main/java/se/bungalow/app/playlist/spec/UNIVERSAL_PLAYLIST_SHARING.md).
