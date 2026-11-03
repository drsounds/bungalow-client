---
title: URI schemes
sidebar_position: 1
---

# URI schemes

Bungalow uses these URI schemes:

| Scheme | Names | Example |
|--------|-------|---------|
| `bungalow:` | views, navigation and library entities | `bungalow:library`, `bungalow:track:42` |
| `sp://` | files inside an app bundle | `sp://modern/skins/modern/images/skin.png` |
| `musik:` | portable, service-independent content ([RFC-0002](../specs/rfc-0002-musik-uri-scheme.md)) | `musik:isrc:GBAYE0601498` |
| `musik:federated:` | decentralised, shareable playlists ([RFC-0001](../specs/rfc-0001-universal-playlist-sharing.md)) | `musik:federated:user:@me@example.social:playlist:<uuid>` |

## Aliases

When you navigate (from the search field, the address field, a link or an
app), `ui::MainWindow` rewrites these prefixes to `bungalow:`:

| Prefix | Becomes |
|--------|---------|
| `space:` | `bungalow:` |
| `musik:` | `bungalow:` |
| `spotify:` | `bungalow:` |
| `spacify:` | `bungalow:` |

So `spotify:` links in legacy apps open in Bungalow. The rewrite is purely a
prefix change. `musik:` URIs aren't interpreted according to RFC-0002 yet; see
[`musik:`](#musik) below.

## `bungalow:` navigation URIs

A view accepts a family of URIs. The view stack shows the **last** registered
view that accepts the URI. See [Navigation](../architecture/navigation.md).

| URI | Shows | Contributed by |
|-----|-------|----------------|
| `bungalow:start` | Start page (`views/start.xml`) | `StartFeature` |
| `bungalow:player`, `bungalow:player:…` | Now Playing | `StartFeature` |
| `bungalow:config` | Settings | `SettingsFeature` |
| `bungalow:library` | Library tracks | `LibraryConcept` |
| `bungalow:localfiles` | Local files | `LibraryConcept` |
| `bungalow:downloads`, `bungalow:downloads#…` | Downloads | `DownloadsFeature` |
| `bungalow:database` | Table manager | `DatabaseFeature` |
| `bungalow:table:<name>` | A user table | `DatabaseFeature` |
| `bungalow:table:<name>:<id>` | One row of a user table | `DatabaseFeature` |
| `bungalow:search`, `bungalow:search:<query>`, `bungalow:search?q=<query>` | Unified search | `SearchConcept` |
| `bungalow:site:<host>[:<path>…][?query][#fragment]` | A web site | `WebConcept` |
| `bungalow:app:<id>[:<segment>…]` | An app bundle, or `https://<id>.apps.bungalow.se` | `WebConcept` |
| `bungalow:store:<name or host>[:<path>…]` | A store page | `WebConcept` |
| `bungalow:curator:<name or host>[:<path>…]` | A curator page | `WebConcept` |
| `bungalow:artist:<isni>` | Artist page (`views/artist.xml`) | `SpaceView` (catch-all) |
| anything else | `views/error.xml` | `SpaceView` (catch-all) |

A `#fragment` selects a tab in tab-bar views, for example `bungalow:start#test`.

Older views that aren't currently registered by a built-in also exist in
`bungalow/src`: `bungalow:user:<id>`, `bungalow:whatsnew`,
`bungalow:internal:toplist` and `bungalow:doctrine:<…>`.

## `bungalow:` entity URIs

These name library data. They are resolved by the [URI store](../space/uri-store.md)
and can be played or bound in markup:

| URI | Resolves to | Playable |
|-----|-------------|----------|
| `bungalow:artist:<isni>` | `{uri, type, isni, name}` | no |
| `bungalow:track:<id>` | `{uri, type, id, name, version, isrc, release, artists}` | yes, `SPApplicationContext::playUri()` |

## `sp://`

`sp://<bundle>/<path>` is a file inside an app bundle. It is resolved by
`SPAppManager::resolve()`, which refuses paths that leave the bundle. In HTML5
apps it is served by the embedded browser. Missing paths without an extension
serve `index.html`. See [App bundles](../apps/bundles.md#sp-uris).

## `musik:`

`musik:` is Bungalow's portable content identifier, specified in
[RFC-0002](../specs/rfc-0002-musik-uri-scheme.md). It names a recording, work,
release, artist or facet without tying it to a service:

```
musik:isrc:<ISRC>                      a recording
musik:iswc:<ISWC>                      a work
musik:upc:<UPC>                        a release
musik:isni:<ISNI>                      an artist
musik:ipi:<IPI>                        a creator
musik:genre:<slug>, musik:mood:<slug>  facets
musik:artist:<artist>[:release:<release>[[:track:<n>]:name:<name>[:version:<version>]]]
musik:track?name=<name>&artist_name=<artist>&release_name=<release>&number=<n>&version=<version>
```

The Java client parses and plays these (`MusikUri`). The C++ client only
rewrites the `musik:` prefix to `bungalow:` (see [Aliases](#aliases)). Its
library already uses the same identifiers (ISRC, UPC, ISNI) as keys. Parsing
the RFC-0002 forms is on the [roadmap](../roadmap.md).

## `musik:federated:`

A Universal Playlist Sharing Link ([RFC-0001](../specs/rfc-0001-universal-playlist-sharing.md))
carries a whole playlist, or a reference to a hosted one, attributed to a
federated handle:

```
musik:federated:user:<@user@instance>:playlist:<uuid>[:<body>][:sig:<signature>]
https://ups.bungalow.net/user/<handle>/playlist/<uuid>[/<body>][?sig=<signature>]
```

The Java client parses UPSLs and shows a preview. The C++ client doesn't handle
them yet. See [Specifications](../specs/index.md).
