# Bungalow Client

BungalownClient is a C++ desktop application.

It uses an own written GUI framework called **Space** which is a lightweight abstract GUI framework that aims to create an alternate protocol for serving content over the internet

## Building

Required libraries:

* nlohmann-json
* Lua
* libpng
* Expat
* wxWidgets with the WebView component enabled (for the wxWidgets backend)

On Windows, the wxWidgets WebView backend also requires the Microsoft Edge
WebView2 Runtime to be installed.

The default UI backend is wxWidgets. On Windows, select `win32` or `wtl` with
`-DBUNGALOW_UI_BACKEND=win32` or `-DBUNGALOW_UI_BACKEND=wtl`. The WTL backend
also requires the WTL headers, supplied with `-DWTL_INCLUDE_DIR=<path>`.

```bash
cmake .
make
```

### HTML5 apps (Chromium Embedded Framework)

On the WTL backend, `-DBUNGALOW_ENABLE_CEF=ON` (on in the `ninja` preset)
runs local HTML5 app bundles in Chromium Embedded Framework. CEF comes from the
overlay port in `ports/cef`, which vcpkg installs into `vcpkg_installed/`:
the prebuilt CEF 151 binary distribution plus a locally built
`libcef_dll_wrapper`. The CEF runtime files are copied next to the executable
after each build.

An app is a folder in `Apps/` (or `~/Bungalow`, `~/Bungalow/Apps`, `~/Documents/Bungalow`, `~/Documents/Bungalow/Apps`)
with a `manifest.json` and an `index.html`:

```json
{
    "BundleType": "Application",
    "BundleIdentifier": "myapp",
    "AppName": { "en": "My App" },
    "AllowedHosts": ["api.example.com"]
}
```

* `spacify:app:myapp:a:b` opens the app at the route `/a/b`. Each app runs in
  its own browser, with isolated storage, which is kept alive while you switch
  views.
* Files are served from `sp://myapp/...`. A missing extensionless path serves
  `index.html`, so single-page apps can use client-side routes.
* An app can only load its own `sp://` files, `data:`/`blob:` URLs, `spacify.se`
  and the https hosts in `AllowedHosts`. You are asked to accept each of those
  hosts once.
* Navigating to another route of a running app calls `window.navigate(path)`.
  A default that uses `history.pushState` is provided; apps may define their
  own. Route changes made by the app appear in the back/forward history.
* `window.spacify.navigate(uri)` opens a `spacify:` URI in the client.
* Theme colors are exposed as `--sp-bg-color`, `--sp-fg-color` and
  `--sp-color-scheme` CSS variables.
* In developer mode the app bar has a DevTools button.

## Usage

TBA

## License

MIT
