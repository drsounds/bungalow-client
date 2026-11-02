---
title: HTML5 apps
sidebar_position: 3
---

# HTML5 apps

An **HTML5 app** is an app bundle with an `index.html`. On the WTL backend with
`-DBUNGALOW_ENABLE_CEF=ON` (on in the `ninja` preset), it runs in the Chromium
Embedded Framework. See [Building](../getting-started/building.md#building-with-cef).

## A minimal app

```
~/Bungalow/Apps/myapp/
├── manifest.json
└── index.html
```

```json
{
    "BundleType": "Application",
    "BundleIdentifier": "myapp",
    "AppName": { "en": "My App" },
    "AllowedHosts": ["api.example.com"]
}
```

Apps are found in `Apps/`, `~/Bungalow`, `~/Bungalow/Apps`,
`~/Documents/Bungalow` and `~/Documents/Bungalow/Apps`. See
[App bundles](./bundles.md#search-paths).

## Routing

* `bungalow:app:myapp:a:b` opens the app at the route `/a/b`.
* Files are served from `sp://myapp/...`. A missing path without an extension
  serves `index.html`, so single-page apps can use client-side routes.
* Navigating to another route of a running app calls `window.navigate(path)`.
  The default implementation uses `history.pushState`; apps can define their
  own.
* Route changes made by the app (`pushState`, hash changes, page loads) appear
  in Bungalow's back/forward history.
* `window.bungalow.navigate(uri)` opens a `bungalow:` URI in the client.
  `bungalow:` links do the same.

## Isolation and security

* **Each app runs in its own browser with its own storage.** The browser is
  kept alive while you switch views, so the app keeps its state.
* An app can load only its own `sp://` files, `data:` and `blob:` URLs,
  `bungalow.se`, the shared API bundle (`sp://import/`) and the https hosts in
  `AllowedHosts`. **You are asked to accept each of those hosts once.**
* If the app tries to open a page outside its bundle, the host is told
  (`externalUrlRequested`) instead of the app navigating away.

## Theme integration

The client's theme is available as CSS variables:

```css
body {
    background: var(--sp-bg-color);
    color: var(--sp-fg-color);
    color-scheme: var(--sp-color-scheme);
}
```

The browser also follows the client's light or dark mode for
`prefers-color-scheme`, native form controls, scrollbars and DevTools.

## Developer mode

When developer mode is on in settings, the app bar has a **DevTools** button.

## The Apps API (`sp://import`)

`Apps/import` is a **Framework** bundle that every app browser also serves at
`sp://import/`. Its files may be read cross-origin. It provides a JavaScript API
compatible with the classic `getSpotifyApi(1)` / `sp.require()` app API:

| URL | What |
|-----|------|
| (injected) `scripts/sp.js` | `getSpotifyApi()`, `sp.require()`, `sp.core` and string helpers |
| `sp://import/scripts/api/models` | `Track`, `Album`, `Artist`, `User`, `Playlist`, `Search`, `Link`, `application`, `player`, `session`, `library`, … |
| `sp://import/scripts/api/views` | `Image`, `Player`, `Track`, `List` |
| `sp://import/scripts/api/auth` | authentication (mocked) |
| `sp://import/css/adam.css`, `api.css` | default dark theme and view layout |

```js
var sp = getSpotifyApi(1);
var models = sp.require('sp://import/scripts/api/models');
console.log(models.application.arguments); // ["a", "b"] for bungalow:app:myapp:a:b
```

All metadata comes from `scripts/api/datasource.js`. For now it is a fictional,
deterministic catalogue (the same URI always gives the same entity), and the
player is simulated. The C++ bridge is meant to replace the functions it
exports (`lookup`, `search`, `createPlaylist`, …) without changing `models` or
`views`.

`spotify:` links and `location = "spotify:..."` are routed through
`window.bungalow.navigate()` as `bungalow:` URIs.

### Developing outside the client

Opened from an `http:` origin, `sp://<bundle>/x` is fetched from
`<origin>/<bundle>/x`. You can serve the `Apps` folder with any static server
and include `scripts/sp.js` in the app page.

## The browser interface

Backends implement `SPEmbeddedBrowser` (`space/include/SPEmbeddedBrowser.h`):

| Member | Purpose |
|--------|---------|
| `Options::developerMode`, `darkColorScheme`, `startupScript`, `allowedHosts` | creation options |
| `Callbacks::routeChanged`, `navigateRequested`, `externalUrlRequested`, `titleChanged` | events from the app |
| `loadPath("/a/b")` | load an in-app path |
| `navigateRoute("/a/b")` | route the running app via `window.navigate` |
| `executeScript`, `reload`, `showDevTools`, `setDarkColorScheme`, `setBounds`, `setVisible` | control |

On the WTL backend it is implemented by `CefAppBrowser`, and `sp://` is served by
`SpSchemeHandler`.
