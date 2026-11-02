---
title: Data and the library
sidebar_position: 7
---

# Data and the library

The library is a local **SQLite** database owned by the user. Content is
described with standard identifiers rather than vendor row ids:

| Entity | Key |
|--------|-----|
| Artist | ISNI |
| Recording | ISRC |
| Release | UPC |

## Where the database lives

`BungalowApp` opens `resources/bungalow.db` next to the executable if it exists.
Otherwise it creates `bungalow.db` in the user data directory
(`%APPDATA%\Bungalow` on Windows). The `Database` class
(`bungalow/include/Database.h`) creates any missing tables on open and migrates
older schemas.

## Schema

| Table | Holds |
|-------|-------|
| `artists` | `isni` (primary key), `name` |
| `recordings` | recordings keyed by ISRC, with name and version |
| `recording_artists` | recording ↔ artist links |
| `labels` | record labels |
| `releases` | releases keyed by UPC, with release date and label |
| `release_artists` | release ↔ artist links, with `is_primary` |
| `tracks` | a recording's position on a release |
| `saves` | what the user saved to their library (soft-deleted with `deleted_at`) |
| `files`, `audio_files`, `track_audio_files` | local files, their audio metadata and cover art, and which track they are |
| `playables`, `playable_tracks` | playable URIs and the tracks they play |
| `downloads` | the download manager's queue and history |
| `bookmarks` | hierarchical site and app bookmarks (see [Sites and bookmarks](../apps/sites-and-bookmarks.md)) |
| `accepted_domains` | hosts the user allowed web content to load from |
| `db_tables` | definitions of user tables |
| `user_<name>` | one table per user-defined table |

The `SP*` classes in `bungalow/include/models` (`SPArtist`, `SPRecording`,
`SPRelease`, `SPTrack`, `SPAudioFile`, `SPPlayable`, …) are the object models
for these tables.

## Importing local files

`LocalFilesView` (`bungalow:localfiles`) lists local audio files.
`AudioImporter` and `MusicScanner` scan folders, read the audio metadata and
fill `files`, `audio_files` and the content tables, so a local file becomes a
recording on a release and can be played as `bungalow:track:<id>`.

## User tables

`DatabaseFeature` adds a small database manager to the client:

* `bungalow:database` opens the **table manager**, where you create tables
  with typed columns.
* Each table appears under the `DATABASE` section of the sidebar and opens at
  `bungalow:table:<name>`.
* Each row opens at `bungalow:table:<name>:<id>`. Row ids are UUIDs.

Table definitions are stored in `db_tables`, and the data in `user_<name>`
tables. Because the sidebar is rebuilt from `getSidebarNodes()`, new tables
appear in it right away.

## Entities in the UI

Views don't read the database directly when they show an entity by URI. They
go through the [URI store](../space/uri-store.md). At startup,
`registerEntityResolvers()` registers resolvers that turn entity URIs into
JSON from the library:

```
bungalow:artist:<isni>   {uri, type, isni, name}
bungalow:track:<id>      {uri, type, id, name, version, isrc, release, artists}
```

The store caches the JSON on disk, so a view shows the last known data straight
away and updates when the resolver returns.
