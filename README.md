# Bungalow Client

BungalowClient is a C++ music and media manager app.

It uses an own written GUI framework called **Space** which is a lightweight abstract GUI framework that aims to create an alternate protocol for serving content over the internet

## Documentation

The documentation is at https://drsounds.github.io/bungalow-client/. Its
source is in [`docs/`](docs/); run `npm install && npm start` there to
preview it locally.

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

* `bungalow:app:myapp:a:b` opens the app at the route `/a/b`. Each app runs in
  its own browser, with isolated storage, which is kept alive while you switch
  views.
* Files are served from `sp://myapp/...`. A missing extensionless path serves
  `index.html`, so single-page apps can use client-side routes.
* An app can only load its own `sp://` files, `data:`/`blob:` URLs, `bungalow.se`
  and the https hosts in `AllowedHosts`. You are asked to accept each of those
  hosts once.
* Navigating to another route of a running app calls `window.navigate(path)`.
  A default that uses `history.pushState` is provided; apps may define their
  own. Route changes made by the app appear in the back/forward history.
* `window.bungalow.navigate(uri)` opens a `bungalow:` URI in the client.
* Theme colors are exposed as `--sp-bg-color`, `--sp-fg-color` and
  `--sp-color-scheme` CSS variables.
* In developer mode the app bar has a DevTools button.

### React apps without a browser (swimming)

`-DBUNGALOW_ENABLE_SWIMMING=ON` (the default) builds the [swimming](swimming/)
library, which runs React (TSX) apps natively on every backend: QuickJS runs
the JavaScript, Yoga lays out the elements and Space draws them. It needs the
`quickjs-ng` and `yoga` packages, which the `swimming` vcpkg feature installs.

An app with `"AppType": "Swimming"` in its manifest is a Vite bundle built with
`vite-plugin-swimming`; `"Main"` names its entry (default `dist/bundle.js`, or
an `.html` file whose classic scripts are run). `bungalow:app:myapp:a:b`
replaces the app's polyfilled history entry with `/a/b` and fires `popstate`,
so React Router follows it. See [Apps/swimmingtest](Apps/swimmingtest/).

The build also makes a `swimming` viewer for the selected backend, which runs
one app in a window, or renders one frame to a PNG with `--out`:

```bash
swimming Apps/swimmingtest/dist/bundle.js --width 500 --height 300
swimming Apps/swimmingtest/dist/bundle.js --out frame.png
```

## Usage

TBA

## License

MIT
