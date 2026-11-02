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

`bungalow/tests/AspectTest.cpp` covers registration, replacement by id and
ordering.

## Compared with the Java client

The Java client uses the same model. There, `ServiceManager` additionally
selects back-ends **by capability** (`getServices(MediaService.class)`). The
C++ client doesn't have a service manager yet: media services are owned by
`BungalowApp` and chosen by URI (`SPService::acceptsUri`). See
[Playback](./playback.md) and [Aspects & Services](../java-client/aspects-and-services.md)
in the Java docs.
