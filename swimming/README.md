# swimming

A C++ port of [swimming](https://github.com/drsounds/swimming), which runs
React (TSX) apps natively, in the spirit of React Native: **no web browser
engine, no WebView, no DOM**. The TypeScript side (`swimming-react`,
`swimming-web-compat`, `vite-plugin-swimming`) is unchanged; this library
replaces the Rust crates, and its GUI backends are Space windows on the
`windows/` (Win32, WTL) and `wxwidgets/` contexts instead of egui and Fastframe.

```
TSX app (Vite bundle) -> swimming-web-compat (window/location/history/timer polyfills)
  -> swimming-react (React reconciler) -> __SWIMMING_NATIVE__ commands
  -> swimming-bridge (QuickJS through the JsEngine interface)
  -> swimming-core (shadow tree, inline CSS, Yoga layout)
  -> SPSwimmingElement (drawn with the Space graphics context)
```

| Rust crate | C++ target | Notes |
| --- | --- | --- |
| `swimming-core` | `swimming-core` | Shadow tree command API, inline CSS parser (in place of lightningcss), UA defaults for all HTML elements, Yoga layout (in place of Taffy), hit testing, the HTML parser the loader uses (in place of scraper) |
| `swimming-render` | `swimming-core` (`Render.h`) | Software painter (in place of tiny-skia) for headless frames: backgrounds, borders, radius; PNG output with libpng |
| `swimming-js` | `swimming-js` (`JsEngine.h`) | Engine-neutral `JsEngine` interface |
| `swimming-js-quickjs` | `swimming-js` (`QuickJsEngine.h`) | QuickJS through quickjs-ng |
| `swimming-bridge` | `swimming-bridge` | `__SWIMMING_NATIVE__`, timers and promise jobs, pointer events, routes, script/HTML/URL loading, the viewer's command line |
| `swimming-bridge` `gui` (egui, Fastframe) | `swimming-space` (`SPSwimmingElement`) and the `swimming` viewer | A Space element that draws the layout, text included, with `SPGraphicsContext` and sends clicks to the app; its timers run through a scheduler each backend installs (`windows/src/SwimmingScheduler.cpp`, `wxwidgets/src/SwimmingScheduler.cpp`) |

`swimming-space` is an interface library: `SPSwimmingElement.cpp` builds as
part of the consumer, against its Space headers and flags.

## Building

This directory is its own CMake project, built from the client's
`CMakeLists.txt` when `BUNGALOW_ENABLE_SWIMMING` is on. It needs
`nlohmann-json`, `libpng`, `quickjs-ng` and `yoga`.

The viewer, `swimming`, is built for the client's `BUNGALOW_UI_BACKEND`
(`windows/swimming/main.cpp` or `wxwidgets/swimming/main.cpp`):

```
swimming <bundle.js|index.html|URL> [--out frame.png] [--width N] [--height N]
```

http(s) sources are fetched with the graphics context's request support, which
is synchronous on Win32; the wxWidgets viewer loads local files only.

## JavaScript engines

`createDefaultEngine()` returns a `QuickJsEngine`. To add an engine, implement
`swimming::JsEngine` and pass it to `swimming::Bridge`; `tests/BridgeTest.cpp`
runs the same conformance checks against an engine.

QuickJS has no `URL`, so the bridge installs a small one, and gives the
stand-in `document` that swimming-web-compat installs a `defaultView`, which
React Router reads.

## Deviations from the Rust version / not yet done

- V8 is not ported; QuickJS is the only engine.
- Text is laid out with approximate metrics (0.55em per character), as in the
  Rust version, but is drawn by the Space graphics context. The headless
  painter still draws no glyphs.
- Layout is Yoga flexbox with HTML UA defaults (block/inline/hidden); Grid and
  `<style>` sheets/selectors are not wired up. Inline CSS covers declarations,
  colours (named, hex, `rgb()`, `hsl()`) and `px`/`%`/`em`/`rem` lengths.
- `<img>` loading and form-control behaviour are not implemented.
