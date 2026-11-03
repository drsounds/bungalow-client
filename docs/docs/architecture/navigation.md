---
title: Navigation and views
sidebar_position: 4
---

# Navigation and views

Every screen is a **view** bound to a family of **`bungalow:` URIs**.
Navigation goes through a single view stack: you ask for a URI and the stack
shows the view that accepts it. Views don't reference each other. The sidebar,
the address bar, a link in a template or an HTML5 app can all navigate by URI.

## `SPViewElement`

```cpp
class SPViewElement : public SPVBoxElement {
public:
    virtual void navigate(const string& uri);         // render this URI
    virtual void navigateBack(const string& uri);     // default: navigate(uri)
    virtual void navigateForward(const string& uri);  // default: navigate(uri)
    virtual bool acceptsUri(const string& uri);       // is this URI mine?
    string getHash();                                 // the #fragment, e.g. a tab id
    virtual void setHash(const string& hash);
};
```

`acceptsUri()` is usually a prefix test or a regular expression, so one view can
serve a family of URIs. For example, `CustomTableEntryView` accepts
`bungalow:table:<name>:<id>`.

## `SPViewStackElement`

```cpp
class SPViewStackElement : public SPVBoxElement {
public:
    void registerView(SPViewElement* view);
    void navigate(const string& uri);
    void goBack();
    void goForward();
    bool canGoBack() const;
    bool canGoForward() const;
    string getCurrentUri() const;
};
```

The stack keeps back and forward history stacks, the active view and the current
URI. It routes a URI to the **last** registered view that accepts it. That is
why `MainWindow` mounts the catch-all `SpaceView` (which accepts everything)
first: every view contributed later wins over it, and `SpaceView` only handles
URIs nobody else claims.

Views stay mounted after their first use, so heavy views such as embedded
browsers keep their state while you navigate elsewhere. `navigateBack()` and
`navigateForward()` let a view restore its own state, for example a browser
going back inside its own history.

:::tip
A view can only be reached once it is registered, through a feature's
`createViews()`, `ConceptContext::registerView()` or
`SPViewStackElement::registerView()`. If no view accepts a URI, the catch-all
`SpaceView` renders it as a template, or `views/error.xml` if there is none.
:::

## The catch-all `SpaceView`

`SpaceView` is an `SPSpiderViewElement`, a tab-bar view. `navigate(uri)` sets
its `src` attribute to the URI, and Space then renders the matching XUL
template from `views/`, for example `bungalow:start` → `views/start.xml` and
`bungalow:artist:…` → `views/artist.xml`. See [Templates](../space/templates.md).

## The `bungalow:` URI space

There is no central registry: a feature or concept chooses its own prefix. URIs
in use:

| URI | View | Contributed by |
|-----|------|----------------|
| `bungalow:start` | `StartView` | `StartFeature` |
| `bungalow:player` | `PlayerView` (now playing) | `StartFeature` |
| `bungalow:config` | `ConfigView` (settings) | `SettingsFeature` |
| `bungalow:library` | `LibraryView` | `LibraryConcept` |
| `bungalow:localfiles` | `LocalFilesView` | `LibraryConcept` |
| `bungalow:downloads` | `DownloadsView` | `DownloadsFeature` |
| `bungalow:database` | `DatabaseManagerView` (table manager) | `DatabaseFeature` |
| `bungalow:table:<name>`, `bungalow:table:<name>:<id>` | user table and row | `DatabaseFeature` |
| `bungalow:site:<host>[:<path>…]` | `SiteView` (web site) | `WebConcept` |
| `bungalow:app:<id>[:<path>…]` | `AppView` (app bundle) | `WebConcept` |
| `bungalow:curator:<name>`, `bungalow:store:<name>` | curator and store pages | `WebConcept` |
| `bungalow:search:<query>`, `bungalow:search?q=<query>` | `SearchView` | `SearchConcept` |
| `bungalow:playlist:<uuid>` | `PlaylistView` | `PlaylistConcept` |
| `bungalow:artist:<isni>`, anything else | template from `views/` | `SpaceView` |

The complete list, including entity URIs, is in
[URI schemes](../reference/uri-schemes.md).

## The header

The header has back and forward buttons, a search field and an address field:

* Text entered in the **search field** that isn't a `bungalow:` URI becomes
  `bungalow:search:<text>`.
* The **address field** is shown when "Full Width" is enabled in settings.
  Anything that isn't a `bungalow:` URI is treated as a search.
* The current URI is mirrored in the address field, or in the search field when
  the address field is hidden.

Elements can also request navigation by firing a `navigate` event on the window
element. `MainWindow` listens for it, and that is how template links and app
bundles navigate.

## The sidebar

`navigation::SidebarNode` is a toolkit-independent tree:

```cpp
navigation::SidebarNode library("Library", "bungalow:library");
library.setIconPath("icons/Library.svg").setExpanded(true);
library.addChild(navigation::SidebarNode("Tracks", "bungalow:library"));
```

* A top-level node **without a URI is a section** (`MAIN`, `LIBRARY`, `APPS`,
  `SITES`, …). Its children are listed under the section header.
* Sections with the same label from different features are **merged**, in the
  order they were first contributed.
* `setBookmark(true)` marks nodes created from user bookmarks, so the sidebar
  can offer to remove them.
* `setSpacer(true)` on a section lists its children after a blank gap instead
  of under a header.
* `setOnRename(handler)` lets the node be renamed in place from its context
  menu; with `setEditOnClick(true)`, clicking the node shows the inline input
  instead of navigating. `setOnMove(handler)` makes it draggable among the
  adjacent movable nodes, and the handler gets its new index among them.
  `setOnRemove(handler)` adds Delete to its context menu.

`ui::LeftLibraryMenu` turns the nodes into items in the `SPTreeViewElement`.
`MainWindow::rebuildNavItems()` rebuilds the whole tree from
`FeatureManager::sidebarNodes()`. It runs at startup and whenever a concept
calls `refreshSidebar()`. Selecting an item navigates to its URI.

### The rootlist

`bungalow:rootlist` is the user's ordered list of playlists, stored in the
`rootlist` table (`uri`, `title`, `position`) with the playlists themselves in
`playlists`. `PlaylistConcept` shows it at the end of the sidebar after a gap,
headed by **New playlist**: clicking it turns its label into an input, and the
submitted name becomes a new playlist at the top of the list. Playlists can be
dragged into a new order, and renamed or deleted from their context menu.
`playlist::RootlistManager` makes the changes and fires `RootlistEvents`, which
the concept follows to refresh the sidebar.
