# Per-app / per-concept database structure

**Status: implemented.** Entity definitions and table creation are owned by each
app/concept rather than declared globally. `DatabaseManager` no longer knows about
any concrete entity — apps can introduce their own tables without touching it.

## How it works

- **`DatabaseManager`** is a thin owner of the SQLite connection plus a DAO
  registry. `init()` only opens the connection (no tables are created up front).
  - `dao(Class<T>)` — returns a cached `Dao`, creating the table if absent
    (`createTableIfNotExists`). Order-independent: reading an entity another
    concept owns auto-ensures its table.
  - `registerEntity(Class<T>)` — same semantics, named for the activation-time
    registration call. It is the seam where a per-app schema-version / migration
    step can later live.

- **Entities live in their owner's `model` package**, not in `db/entity` (removed):

  | Owner concept | Package | Entities |
  | --- | --- | --- |
  | `music` (shared core) | `app/music/model` | abstract bases `Node`, `Content`, `Creator`, `Release`, `ContentCollection`, `ContentCollectionRow`, `CreatorCredit`, `Playable`; concretes `Artist`, `MusicWork`, `Recording`, `RecordingFile`, `MusicRelease`, `Track`, `RecordingCreatorCredit`, `ReleaseCreatorCredit` |
  | `playlist` | `app/playlist/model` | `Playlist`, `PlaylistRow` |
  | `downloads` | `app/downloads/model` | `Download` |
  | `web` | `app/web/model` | `Bookmark` |
  | `media` | `app/media/model` | `MusicServiceTrack` |
  | `localmusic` | `app/localmusic/model` | `LocalFile` |

  The shared content/credit/collection hierarchy stays together under
  `music/model` as one foundation; other concepts depend on those bases
  cross-package (e.g. `PlaylistRow extends ContentCollectionRow<Content<?>>`).

- **Registration is per concept, not per application.** `registerEntity` is
  exposed on `ConceptContext` only (an `Application` may own several concepts).
  Each concept registers its own tables in `onActivate`:

  ```java
  // se.spacify.app.music.concept.MusicConcept
  @Override public void onActivate(ConceptContext ctx) {
      ctx.registerEntity(Artist.class);
      ctx.registerEntity(Recording.class);
      // … the rest of the shared music model
  }
  ```

  Tables persist across deactivation (user data must survive disabling a plugin),
  so concept teardown does **not** drop tables.

## Adding a new entity

1. Put the `@DatabaseTable` class in the owning concept's `model` package.
2. `ctx.registerEntity(MyEntity.class);` in that concept's `onActivate`.
3. Access it anywhere via `DatabaseManager.getInstance().dao(MyEntity.class)`.

A brand-new concept (and thus a new vertical's tables) needs no change to
`DatabaseManager` or to any other app.

> Note: there is no data-migration engine yet — only `createTableIfNotExists`.
> Table names are unchanged from the previous global schema, so existing
> `~/.spacify/library.db` files keep working.
