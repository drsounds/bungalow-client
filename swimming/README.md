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
  -> swimming-bridge (QuickJS or V8 through the JsEngine interface)
  -> swimming-core (shadow tree, inline CSS, Yoga layout)
  -> SPSwimmingElement (drawn with the Space graphics context)
```

| Rust crate | C++ target | Notes |
| --- | --- | --- |
| `swimming-core` | `swimming-core` | Shadow tree command API, inline CSS parser (in place of lightningcss), UA defaults for all HTML elements, Yoga layout (in place of Taffy), hit testing, the HTML parser the loader uses (in place of scraper) |
| `swimming-render` | `swimming-core` (`Render.h`) | Software painter (in place of tiny-skia) for headless frames: backgrounds, borders, radius; PNG output with libpng |
| `swimming-js` | `swimming-js` (`JsEngine.h`) | Engine-neutral `JsEngine` interface; `createEngine(name)` / `engineNames()` |
| `swimming-js-quickjs` | `swimming-js` (`QuickJsEngine.h`) | QuickJS through quickjs-ng |
| `swimming-js-v8` | `swimming-js` (`V8Engine.h`), with `SWIMMING_ENABLE_V8` | V8 15.2 from rusty_v8's prebuilt library (the `rusty-v8` overlay port), the build the Rust crate uses |
| `swimming-bridge` | `swimming-bridge` | `__SWIMMING_NATIVE__`, timers and promise jobs, pointer events, routes, script/HTML/URL loading, the viewer's command line |
| `swimming-bridge` `gui` (egui, Fastframe) | `swimming-space` (`SPSwimmingElement`) and the `swimming` viewer | A Space element that draws the layout, text included, with `SPGraphicsContext` and sends clicks to the app; its timers run through a scheduler each backend installs (`windows/src/SwimmingScheduler.cpp`, `wxwidgets/src/SwimmingScheduler.cpp`) |

`swimming-space` is an interface library: `SPSwimmingElement.cpp` builds as
part of the consumer, against its Space headers and flags. Executables that
run apps call `swimming_host_executable(<target>)` for an 8 MiB main thread
stack (see below).

## Building

This directory is its own CMake project, built from the client's
`CMakeLists.txt` when `BUNGALOW_ENABLE_SWIMMING` is on. It needs
`nlohmann-json`, `libpng`, `quickjs-ng` and `yoga`, and `rusty-v8` for V8.

The viewer, `swimming`, is built for the client's `BUNGALOW_UI_BACKEND`
(`windows/swimming/main.cpp` or `wxwidgets/swimming/main.cpp`):

```
swimming <bundle.js|index.html|URL> [--engine quickjs|v8] [--out frame.png] [--width N] [--height N]
```

http(s) sources are fetched with the graphics context's request support, which
is synchronous on Win32; the wxWidgets viewer loads local files only.

## JavaScript engines

QuickJS is always built. V8 is built with `SWIMMING_ENABLE_V8` (the client's
`BUNGALOW_ENABLE_V8`, which adds the `v8` vcpkg feature; Windows x64).
`SWIMMING_DEFAULT_ENGINE` (`quickjs` or `v8`) picks the engine
`createDefaultEngine()` returns; as in the Rust version, QuickJS is the default
when both are built. `tests/BridgeTest.cpp` and `bungalow-swimming-app-test`
run every check against every engine built in.

Neither engine has `URL`, so the bridge installs a small one, and gives the
stand-in `document` that swimming-web-compat installs a `defaultView`, which
React Router reads.

Both engines stop JS recursion 128 KiB short of the thread's real stack end
(`src/StackLimit.cpp`), so it throws a `RangeError` instead of crashing; React
renders recursively, hence the bigger main thread stack.

### How V8 is linked

There is no V8 port in vcpkg, and building V8 needs a Chromium toolchain, so
`ports/rusty-v8` installs the static library rusty_v8 publishes for the Rust
`v8` crate. It contains Chromium's libc++ (in `std::__Cr`) and is built with
`/Zl`, so it links with MSVC code on either CRT, but V8's C++ API, whose
signatures use `std` types, is not usable from MSVC. `V8Engine` calls
rusty_v8's `extern "C"` binding layer instead, declared in `V8Engine.cpp`,
with `Isolate::CreateParams` and the scope objects laid out as rusty_v8 lays
them out (checked against `v8__Isolate__CreateParams__SIZEOF`).

The library also expects symbols that the Rust side of the `v8` crate and the
`temporal_rs` crate provide. `V8RustStubs.cpp` defines them as stubs that stop
the process: `V8Engine` uses none of those features (custom platforms, the
inspector, serializer delegates, cppgc), and starts V8 with
`--no-harmony-temporal`, so there is no `Temporal` global, as with QuickJS.

## Deviations from the Rust version / not yet done

- V8 is Windows x64 only; rusty_v8 publishes macOS and Linux libraries too,
  which the port could add.
- No `Temporal` on V8 (see above).
- Text is laid out with approximate metrics (0.55em per character), as in the
  Rust version, but is drawn by the Space graphics context. The headless
  painter still draws no glyphs.
- Layout is Yoga flexbox with HTML UA defaults (block/inline/hidden); Grid and
  `<style>` sheets/selectors are not wired up. Inline CSS covers declarations,
  colours (named, hex, `rgb()`, `hsl()`) and `px`/`%`/`em`/`rem` lengths.
- `<img>` loading and form-control behaviour are not implemented.
