---
title: URI store
sidebar_position: 4
---

# URI store

`space::SPUriStore` (`space/include/SPUriStore.h`) is a small global store, in
the style of Redux, for **URI entities**: the data behind URIs such as
`bungalow:artist:<isni>` or `bungalow:track:<id>`. It provides
**stale-while-revalidate** loading: a view shows the last known data at once,
from memory or the JSON cache on disk, while a resolver fetches fresh data in
the background.

## Entries

The state is a map of URI → `SPUriEntry`:

```cpp
enum class SPUriStatus { Empty, Cached, Loading, Fresh, Error };

struct SPUriEntry {
    nlohmann::json data;        // null until something is known
    SPUriStatus status = SPUriStatus::Empty;
    int64_t updatedAt = 0;      // ms since epoch of the data
    uint64_t requestId = 0;     // fetch in flight / last fetch
    std::string error;
};
```

| Status | Meaning |
|--------|---------|
| `Empty` | nothing is known yet |
| `Cached` | loaded from the disk cache, not yet revalidated |
| `Loading` | a resolver is fetching; `data` still holds the last known value |
| `Fresh` | fetched during this session |
| `Error` | the last fetch failed; `data` still holds the last known value |

## Actions and the reducer

State only changes through `dispatch(SPUriAction)`, which applies the pure
function `SPUriStore::reduce(entry, action)`:

| Action | Effect |
|--------|--------|
| `Hydrate` | data loaded from disk |
| `Pending` | a fetch started (`requestId`) |
| `Fulfilled` | a fetch finished with a payload |
| `Rejected` | a fetch failed with an error |
| `Patch` | merge the payload into the current data (optimistic update) |
| `Invalidate` | mark stale, so the next `resolveURI` fetches again |
| `Evict` | forget the entity, including its cache file |

Results of older requests are ignored by `requestId`, so a slow response can't
overwrite a newer one.

## API

```cpp
auto& store = SPUriStore::instance();

// Register a resolver for a prefix; the longest matching prefix wins.
store.registerResolver("bungalow:artist:",
    [](const std::string& uri, auto resolve, auto reject) {
        // fetch on any thread, then:
        resolve(nlohmann::json{{"name", "…"}});
    });

// What is known now. Starts a fetch if the entry is missing, cached,
// invalidated, failed, or older than maxAgeMs.
SPUriEntry entry = store.resolveURI("bungalow:artist:0000000121032683");

// Listen to one URI (or every URI with ""). Unsubscribes when destroyed.
SPUriStore::Subscription sub = store.subscribe(uri, [](const std::string&, const SPUriEntry& e) { … });
```

Other members: `get(uri)` (hydrates from disk on first access),
`setPersistenceDirectory(dir)`, `setScheduler(fn)`, `setClock(fn)`,
`setDefaultMaxAge(ms)`, `reset()` and `cacheFilePath(uri)`.

## Threading

The store isn't thread-safe and is used from the UI thread. Resolvers may
complete on any thread. Their results are passed to the **scheduler**, which
the application points at its UI thread (`space::win::postToUi` on
Windows, `CallAfter` on wxWidgets).

## In the application

At startup, `BungalowApp`:

* sets the persistence directory to `<user data>/cache/uris`,
* calls `registerEntityResolvers(store, database)`, which resolves
  `bungalow:artist:<isni>` and `bungalow:track:<id>` from the library database.

## In markup

Two element attributes use the store:

```xml
<!-- Render a whole template for a URI; re-rendered when its entity changes -->
<vbox src="bungalow:artist:0000000121032683" />

<!-- Bind one text to a field; shows the fallback until data arrives -->
<text entity="bungalow:artist:0000000121032683" entity-field="name">Loading…</text>
```

With `entity`, the element also gets an `entity-status` attribute with the
current status. `entity-field` defaults to `name`.
