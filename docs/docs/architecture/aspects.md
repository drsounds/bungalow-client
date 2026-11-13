---
title: Aspects
sidebar_position: 2
---

# Aspects

Everything pluggable in Bungalow, including features, concepts and search
providers, is an **aspect**: an object with a stable identity that is told when
it is registered. Each kind of aspect has its own **manager**.

Headers: `bungalow/include/aspect/Aspect.h`, `AspectManager.h`,
`BaseAspectManager.h`.

## `Aspect`

```cpp
namespace space::aspect {

class Aspect {
public:
    virtual ~Aspect() = default;
    virtual std::string getId() const = 0;
    virtual std::string getName() const = 0;
    /// Called once by the manager the aspect was registered with.
    virtual void onRegister(AspectManagerBase& manager) {}
};

}
```

## `AspectManager<T>`

A registry of one kind of aspect, keyed by `getId()`. **Managers own the
aspects registered with them.**

```cpp
template <class T>
class AspectManager : public AspectManagerBase {
public:
    virtual T* registerAspect(std::unique_ptr<T> aspect) = 0; // returns the raw pointer
    virtual void unregisterAspect(const std::string& id) = 0; // destroys it
    virtual T* get(const std::string& id) const = 0;
    virtual std::vector<T*> all() const = 0;                  // registration order
};
```

`AspectManagerBase` is the untyped base passed to `Aspect::onRegister()`.

## `BaseAspectManager<T>`

The default implementation is an insertion-ordered
`std::vector<std::unique_ptr<T>>`:

* `registerAspect()` ignores null. If an aspect with the same id is already
  registered, it calls the **virtual** `unregisterAspect()` first, so a
  subclass can undo whatever the old aspect added. It then appends the new
  aspect and calls `onRegister(*this)`.
* `unregisterAspect()` erases and destroys the aspect.
* `get()` and `all()` look aspects up.

Concrete managers extend it and override `registerAspect()` and
`unregisterAspect()` to wire aspects into the running application and remove
them again:

| Manager | Aspect | What registering does |
|---------|--------|-----------------------|
| `feature::FeatureManager` | `Feature` | mounts the feature's views in the view stack |
| `concepts::ConceptManager` | `Concept` | activates the concept with a recording context |
| `search::SearchManager` | `SearchProvider` | adds the provider to unified search |
| `service::ServiceManager` | `Service` | calls `onCreate()`; unregistering calls `onStop()` and `onDestroy()` |

`bungalow/tests/AspectTest.cpp` covers registration, replacement by id and
ordering.

## Services and capabilities

A **Service** is a back-end: an aspect with a lifecycle (`onCreate`,
`onStart`, `onStop`, `onDestroy`). What it can do is expressed by also
deriving from **capability** interfaces, so one object can combine several.
Headers: `bungalow/include/service/`.

| Capability | Responsibility |
|------------|----------------|
| `MediaService` | load a URI it `acceptsUri()`, play / pause / stop / seek, playback listeners |
| `MusicService` | a `MediaService` that also looks tracks up by ISRC or title and artist, without loading them |
| `MusicCatalogueService` | search and browse a catalogue (artists, releases, recordings) |
| `AuthAspect` | sign in and out. Every `Service` derives from it with empty defaults; one with accounts overrides `supportsLogin()`, `login()`, `logout()`, `isLoggedIn()` and `getSession()` |

Callers never name a vendor. They ask the `ServiceManager` for a capability:

```cpp
for (MusicService* ms : services.getServices<MusicService>()) { … }
services.login("some.service", onSuccess, onError);   // asynchronous; the session lands in services.accounts()
```

Both use `dynamic_cast`, so a Service is offered for every interface it
implements. The capabilities derive from `Service` **virtually**, so a class
implementing `MusicService` and `MusicCatalogueService` has a single
`Service` base. A `Service*` therefore can't be `static_cast` down to its
class; use `registerService(std::unique_ptr<T>)`, which returns the typed
pointer.

`BungalowApp` owns the `ServiceManager` and registers `LocalMediaService`
(id `local`). See [Playback](./playback.md) for how a track is resolved to a
service, and [Aspects & Services](../java-client/aspects-and-services.md) for
the Java client, which the C++ model follows.
