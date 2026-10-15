---
layout: default
title: Navigation & Views
nav_order: 5
---

# Navigation & Views

Every screen in Bungalow is a **View** bound to a **`spacify:` URI**. Navigation
is a single-stack router: ask to go to a URI, and the first registered view that
accepts it is shown. This keeps screens decoupled — a plugin contributes a view
and a URI, and anything (sidebar, header, another view, a deep link) can navigate
to it without a direct reference.

Types: [`View`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/navigation/View.java),
[`ViewStack`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/navigation/ViewStack.java),
[`SidebarNode`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/navigation/SidebarNode.java),
`NavigationListener`, `PlayerView`.

## The `View` contract

A View **is** a [`Panel`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/controls/Panel.java) (a Swing
component), so the stack mounts and shows/hides the View itself — no manual
component swapping:

```java
public abstract class View extends Panel {
    public View(ViewStack viewStack) { … }           // sets a BorderLayout
    public abstract boolean acceptsUri(String uri);   // "is this URI mine?"
    public abstract void    navigate(String uri);     // "render this URI"
    public JComponent getComponent() { return this; } // the View itself by default
    public String getName() { return ""; }
    public void onShow() {}  public void onHide() {}

    // Views with their own history (e.g. the embedded browser) opt in:
    public boolean handlesHistory() { return false; }
    public boolean canGoBack() / canGoForward() / goBack() / goForward();
}
```

Render directly into `this`. Legacy views that build an internal panel may still
override `getComponent()` to return it — the ViewStack wraps that panel inside the
View on first show (`View.mount()`), so the View is always the thing mounted.

`acceptsUri` is typically a regex or prefix test, so one view can serve a family
of URIs (e.g. `ReleaseDetailView` accepts `spacify:library:release:<id>`).

## The `ViewStack` router

[`ViewStack`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/navigation/ViewStack.java) is a `Panel`
using a **`CardLayout`**: each view is mounted once as a card, and the layout
shows exactly one and hides the rest — so show/hide is automatic. It also keeps
the current view, the registered views, and back/forward history.

`navigate(uri)`:

1. Find the first registered view with `acceptsUri(uri) == true` (no match → no-op).
2. If it's the **same** view (different URI) → retarget it in place via
   `view.navigate(uri)` (no card switch).
3. Otherwise `onHide()` the old view, mount the matched view as a card if needed,
   `navigate` + `onShow()` it, `CardLayout.show` it (which hides the previous),
   push history, and notify `NavigationListener`s.

Because every view stays mounted as its own card, heavyweight views (the JCEF
browser, the now-playing view) keep their state across navigation instead of
being rebuilt.

Back/forward first defer to a view that `handlesHistory()` (the browser consumes
its own back), then fall back to the URI history stacks. `NavigationListener`s
(the header's address field, the back/forward buttons, the immersive-mode
toggle) are notified on every change.

> A view is only reachable once it is **registered** with the stack — via
> `ctx.registerView(v)` from a plugin, or `viewStack.registerView(v)` for views
> the window owns (e.g. the now-playing view). An unregistered view means
> `navigate` silently matches nothing.

## The `spacify:` URI scheme

URIs are opaque strings matched by views; there is no central registry, which is
what keeps the scheme open — a plugin coins its own namespace. Conventions in use:

| URI pattern | View | Provided by |
|-------------|------|-------------|
| `spacify:now-playing`, `spacify:home` | Now Playing | window / media plugin |
| `spacify:library`, `:tracks` `:recordings` `:releases` `:artists` `:local` | library lists | `library` |
| `spacify:library:release:<id>`, `spacify:library:artist:<id>` | detail pages | `library` |
| `spacify:recording:isrc:<isrc>` | recording resolver | `library` / music |
| `spacify:playlist:<id>` | playlist | `playlist` |
| `spacify:search`, `spacify:search?q=…` | search | `search` |
| `spacify:catalog:<serviceId>:artists\|releases\|recordings` | catalogue browse | `catalog` |
| `spacify:site:<host>`, `spacify:store:<host>` | embedded browser / store | `web` |
| `spacify:youtube:<id>` | YouTube playback | `youtube` |
| `spacify:plugins` | plugin manager | UI |

Query strings (`?q=…`, `?artist=…`) carry view parameters; the accepting view
parses them in `navigate`.

## Rendering view content with Spider

A view's `navigate` has to produce the actual widgets. It can build them in Java,
or it can delegate to **[Spider](spider-templates.html)** — an ASP/JSP-style template
engine that turns Lua-preprocessed XUL markup into a control tree and re-renders on
postback. A controller yields markup for the URI; the view mounts it via
`Control.setInnerXul`, which reconciles against the previous render so widget state
survives. [`TabBarView`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/navigation/TabBarView.java)
pairs naturally with it: a `<view>`'s `<page>`s become its tabs, and a URI
**`#fragment`** selects the tab whose id matches (e.g. `spacify:testapp#overview`).
The [Test App](spider-templates.html#worked-example-the-test-app) is a full example.

## The sidebar

[`SidebarNode`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/navigation/SidebarNode.java) is a
plain tree of `{ label, uri, children, icon }`. A leaf with a URI navigates on
click; a branch groups children. Plugins contribute subtrees via
`ctx.addSidebarNode(node)`, which returns a `SidebarHandle` for **post-activation
edits** — adding/removing children as state changes, and `expand()`.

The sidebar itself (`LeftLibraryMenu`) is **owned by `MainWindow`**, created
before plugins activate, and then embedded by the active Chrome. That ownership
split is deliberate: the Chrome can change at runtime (switch Design) without
losing the plugin-contributed nodes. See [Look & feel](look-and-feel.html).
