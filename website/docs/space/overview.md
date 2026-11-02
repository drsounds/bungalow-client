---
title: Space overview
sidebar_position: 1
---

# The Space UI framework

**Space** is the UI framework Bungalow is built on. It lives in `space/` and
doesn't depend on any specific toolkit. Bungalow's shell, views and skins are
Space elements, and a backend (wxWidgets, Win32 or WTL) only supplies a window,
input events and a graphics context.

Space was designed as an alternative way to describe and serve app content:
markup and templates are requested by URI, like pages from a server, but are
rendered with native elements instead of a browser.

## Building blocks

| Piece | Types | Page |
|-------|-------|------|
| Element tree | `SPNode`, `SPElement`, `SPVBoxElement`, `SPHBoxElement`, `SPWindowElement`, … | [Elements and XUL](./elements-and-xul.md) |
| Markup | XUL-like XML parsed by `SPXmlParser` (Expat) | [Elements and XUL](./elements-and-xul.md) |
| Templates | `SPTemplate` (Lua), `src` and `entity` attributes | [Templates](./templates.md) |
| Data | `SPUriStore`, a Redux-like store of URI entities with a disk cache | [URI store](./uri-store.md) |
| Look | `SPSkin` and its subclasses, skin bundles (`SPXmlSkin`) | [Skins](./skins.md) |
| Drawing | `SPGraphicsContext`, implemented per backend | [Backends](./backends.md) |
| Navigation | `SPViewElement`, `SPViewStackElement`, `SPTabBarViewElement` | [Navigation](../architecture/navigation.md) |
| Apps | `SPAppManager`, `SPAppBundle`, `SPNativeApp`, `SPEmbeddedBrowser` | [Apps](../apps/bundles.md) |
| Host services | `SPApplicationContext` (playback) | [Playback](../architecture/playback.md) |

## How a screen is produced

```
 views/start.xml                 SPTemplate                 setInnerXUL
 (XUL + ${…} + % lines)  ──►   Lua renders markup   ──►   element tree   ──►  skin draws it
        ▲                         with model + request         (reconciled)      through the
        │                                ▲                                       graphics context
  bungalow:start ── src ──┘     SPUriStore entity data
```

1. A view gets a `bungalow:` URI and sets it as an element's `src`.
2. The element loads the template for the route and renders it with
   [`SPTemplate`](./templates.md). The model includes the URI's entity from the
   [URI store](./uri-store.md).
3. The resulting XUL is turned into elements with `setInnerXUL`.
4. When the store gets fresh data for the URI, the template is rendered again.
5. Elements are laid out (`pack()`) and painted by the active
   [skin](./skins.md) through the backend's graphics context.

## Element basics

Every element has:

* **Identity**: `id`, a class list and a type (`getType()`, for example
  `"button"` or `"viewstack"`).
* **Layout**: width and height in pixels or percent, `flex`, `gap`, margins and
  padding. `vbox` and `hbox` lay their children out in a column or a row.
* **Style**: background and foreground colour, tint, gradient, colour scheme
  (default, dark, light) and state (normal, hover, active).
* **Events**: `addEventListener(name, callback, …)` for `click`, `enter`,
  `navigate`, `itemselected`, `attributechanged` and others. Mouse capture is
  available for drags.
* **Content**: `loadXUL(file)`, `setInnerXUL(xml)` and the `src` and `entity`
  attributes.
