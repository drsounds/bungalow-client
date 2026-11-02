---
title: Features and concepts
sidebar_position: 3
---

# Features and concepts

Two kinds of contribution make up the client's screens:

* A **feature** is a few screens plus their sidebar navigation.
* A **concept** is a domain (the library, web browsing, search) with a
  lifecycle. When it is activated it contributes features, views and search
  providers, and when it is removed everything it added goes with it.

Use a feature for "some views and a sidebar entry". Use a concept when the
domain needs activation logic, live state or listeners.

## Feature

```cpp
namespace space::feature {

class Feature : public aspect::Aspect {
public:
    /// Views to mount in the view stack when the feature is registered.
    /// Create them with viewStack as their parent.
    virtual std::vector<SPViewElement*> createViews(SPViewStackElement* viewStack) { return {}; }

    /// Sidebar nodes. Called every time the sidebar is rebuilt, so features
    /// with live content (bookmarks, custom tables) return their current state.
    virtual std::vector<navigation::SidebarNode> getSidebarNodes() const { return {}; }
};

}
```

`FeatureManager::registerAspect()` calls `createViews()` and registers each view
with the view stack. It remembers them by feature id so that
`unregisterAspect()` can detach and hide them again. `sidebarNodes()` returns
every feature's nodes in registration order.

### Example: the Start feature

```cpp
class StartFeature : public feature::Feature {
public:
    std::string getId() const override   { return "start"; }
    std::string getName() const override { return "Start"; }

    std::vector<SPViewElement*> createViews(SPViewStackElement* viewStack) override {
        return {new views::PlayerView(viewStack), new views::StartView(viewStack)};
    }

    std::vector<navigation::SidebarNode> getSidebarNodes() const override {
        navigation::SidebarNode main("MAIN");
        main.addChild(navigation::SidebarNode("Start", "bungalow:start").setIconPath("icons/NowPlaying.svg"));
        main.addChild(navigation::SidebarNode("Now Playing", "bungalow:player").setIconPath("icons/NowPlaying.svg"));
        return {main};
    }
};
```

## Concept

```cpp
namespace space::concepts {

class Concept : public aspect::Aspect {
public:
    virtual void onActivate(ConceptContext& ctx) = 0;
    /// Releases whatever onActivate() acquired outside the context (listeners, …).
    virtual void onDeactivate() {}
    /// Icon shown for the concept; empty falls back to a default.
    virtual std::string getIconPath() const { return {}; }
};

}
```

### `ConceptContext`

`onActivate()` gets a registration surface. **Every contribution made through
it is recorded**, so unregistering the concept removes exactly what it added:

| Method | Effect |
|--------|--------|
| `conceptId()` | id of the concept being activated |
| `registerFeature(std::unique_ptr<Feature>)` | registers a feature; its views and sidebar nodes are wired automatically |
| `registerView(SPViewElement*)` | mounts a view in the `bungalow:` URI space (create it with `viewStack()` as parent) |
| `registerSearchProvider(std::unique_ptr<SearchProvider>)` | adds a provider to unified search |
| `viewStack()` | the app's view stack |
| `searchManager()` | the search manager |
| `refreshSidebar()` | rebuilds the sidebar from every feature's current nodes |

### `ConceptManager`

The manager is built with a `Host`, which is everything a concept may
contribute to:

```cpp
struct Host {
    SPViewStackElement* viewStack = nullptr;
    feature::FeatureManager* features = nullptr;
    search::SearchManager* search = nullptr;
    std::function<void()> refreshSidebar;
};
```

* `registerAspect(concept)` creates a private `RecordingContext` for the
  concept and calls `onActivate(context)`.
* `unregisterAspect(id)` calls `onDeactivate()` and then undoes the recorded
  contributions: features are unregistered, views detached and search
  providers removed.
* The destructor deactivates every concept in reverse order without touching
  the UI, which is being torn down at that point.

### Example: the Library concept

```cpp
void LibraryConcept::onActivate(concepts::ConceptContext& ctx) {
    ctx.registerFeature(std::make_unique<LibraryFeature>());          // views + LIBRARY section
    ctx.registerSearchProvider(std::make_unique<LibrarySearchProvider>());
}
```

### Example: live sidebar content (the Web concept)

`WebConcept` registers `SiteView`, `AppView`, `CuratorView` and `StoreView` and
a `BookmarksFeature` whose `getSidebarNodes()` reads the bookmarks table every
time it is called. It also subscribes to `web::BookmarkEvents`. When a bookmark
changes it calls `ctx.refreshSidebar()` and fetches the favicon of a newly
bookmarked host. Because the listener was acquired outside the context, it is
released in `onDeactivate()`:

```cpp
void WebConcept::onDeactivate() {
    web::BookmarkEvents::removeListener(m_bookmarkListener);
    m_bookmarkListener = 0;
    m_ctx = nullptr;
}
```

## Getting at app services

Built-in features build their views with services of the running app.
`app/AppContext.h` exposes them; each is null until the app has opened it:

```cpp
namespace space::app {
Database* database();
sqlite3* databaseHandle();
DownloadManager* downloadManager();
}
```

## Adding a built-in

1. Create `bungalow/include/app/<domain>/` and `bungalow/src/app/<domain>/`.
2. Implement a `Feature` or a `Concept`.
3. Register it in `app::registerBuiltins()` (`bungalow/src/app/BuiltinRegistry.cpp`)
   at the position where its sidebar section should appear.

CMake picks up new files in the package directories automatically
(`file(GLOB_RECURSE … CONFIGURE_DEPENDS)`).
