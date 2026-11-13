---
title: Plugins
sidebar_position: 7
---

# Plugins

A plugin is a shared library, or a Python file, that adds **Services** to the client: a streaming
back-end, a store, anything with a sign-in or playback. It talks to the client
only through COM interfaces, so it can be built with any compiler and shipped
separately from the application.

This is the native counterpart of the legacy Java `Application` /
`ApplicationContext` / `ApplicationManager` design.

| Legacy Java | Native |
|-------------|--------|
| `Application` (plugin entry class) | `com::IApplication` |
| `ApplicationContext` (recording registration surface) | `com::IApplicationContext` |
| `ApplicationManifest` (id, name, version in the jar manifest) | `IApplication::GetId` / `GetName` / `GetVersion` |
| `ApplicationLoader` + `ApplicationManager` | `plugin::PluginManager` |
| jars in `<app>/apps` and `~/Bungalow` | libraries in `<app>/Apps` and `~/Bungalow` |

Headers: `bungalow/include/plugin/`.

## The interfaces

`BungalowPlugin.h` is the whole contract. It is self-contained (no STL, no
other Bungalow header) and mirrors the native classes:

| Native class | COM interface |
|--------------|---------------|
| `aspect::Aspect` | `IAspect` |
| `service::Service` | `IService` |
| `service::AuthAspect` | `IAuthAspect`, with `ISession` and `ILoginCallback` |
| `service::MediaService` | `IMediaService`, with `IPlaybackListener` |
| `service::MusicService` | `IMusicService` |
| `feature::Feature` | `IFeature` |
| `concepts::Concept` | `IConcept`, with `IConceptContext` |

The usual COM rules apply: `IUnknown` is binary compatible with the Windows
one, every method returns an HRESULT-style `Result`, interface out parameters
come back AddRef'ed, strings are UTF-8 (`IString` going out), and nothing
throws across the boundary. Capabilities are discovered with `QueryInterface`:
a service that plays music answers `IID_IMediaService` and `IID_IMusicService`,
one with accounts answers `IID_IAuthAspect`.

`PluginSupport.h` is optional and header-only: `ComPtr`, a `ComObject<…>` base
that supplies reference counting and `QueryInterface`, and string helpers.

:::note
`IFeature` and `IConcept` are defined, but the host does not accept them yet:
`RegisterFeature` and `RegisterConcept` return `kNotImpl`. Native views cannot
cross the plugin boundary.
:::

## Writing a plugin

A plugin exports one function and returns its `IApplication` from it:

```cpp
#include "plugin/BungalowPlugin.h"
#include "plugin/PluginSupport.h"
using namespace space::com;

class MyService : public ComObject<IService, IAuthAspect, IMediaService, IMusicService> { … };

class MyApplication : public ComObject<IApplication> {
    Result BUNGALOW_CALL GetId(IString** out) override { return returnString("com.acme.stream", out); }
    …
    Result BUNGALOW_CALL OnActivate(IApplicationContext* context) override {
        auto service = make<MyService>();
        return context->RegisterService(service.get());
    }
};

BUNGALOW_PLUGIN_EXPORT Result BungalowCreateApplication(uint32_t abiVersion, IApplication** out) {
    if (abiVersion != kAbiVersion) return kFail;
    *out = new MyApplication();
    return kOk;
}
```

`bungalow/examples/plugin/SamplePlugin.cpp` is a complete one; the test suite
builds it as `bungalow-sample-plugin` and loads it.

Put the library in `<app>/Apps` or `~/Bungalow`. Libraries directly inside
those folders are loaded at startup; ones that do not export
`BungalowCreateApplication` are skipped.

## Writing a plugin in Python

The lightweight alternative: a plugin can be one `.py` file. Bungalow embeds a
Python interpreter (pybind11; `BUNGALOW_ENABLE_PYTHON`, on by default), so
nothing has to be compiled or installed.

```python
import bungalow

ID, NAME, VERSION = "com.acme.stream", "Acme", "1.0.0"   # optional

class Acme(bungalow.Service):
    id, name = "acme.stream", "Acme Stream"

    def login(self, done):                       # has accounts
        done.success(bungalow.Session("u1", "Me", {"token": "…"}))

    def lookup(self, isrc):                      # plays music
        return bungalow.Track("acme:track:1", "Song", "Artist")

    def accepts_uri(self, uri): return uri.startswith("acme:")
    def load_uri(self, uri): self.playback.state(bungalow.LOADING)
    def play(self): self.playback.state(bungalow.PLAYING)

def activate(context):
    context.register_service(Acme())
```

A plugin is a `.py` file or a package folder (with an `__init__.py`, so it can
ship helper modules) directly inside `<app>/Apps` or `~/Bungalow`. Names
starting with `_` or `.` are skipped. `bungalow/examples/plugin/sample_plugin.py`
is a complete one.

| Module level | |
|--------------|---|
| `activate(context)` | required; call `context.register_service(service)`. `context.id` is the plugin id |
| `deactivate()` | optional |
| `ID`, `NAME`, `VERSION` | optional; the id defaults to `py.<file name>` |

`bungalow.Service` has a default for every method. What a subclass overrides
decides what the service can do:

| Override | The service… |
|----------|--------------|
| `login(done)`; optionally `logout()`, `is_logged_in()`, `session()` | has accounts. Call `done.success(bungalow.Session(user_id, user_name, data))` or `done.error(message)`, at once or later, from any thread. `data` is anything `json.dumps` takes and is encrypted by the host |
| `load_uri(uri)`, `accepts_uri(uri)`, `play()`, `pause()`, `stop()`, `seek(ms)`, `position_ms()`, `duration_ms()` | plays media. Report through `self.playback`: `state(bungalow.PLAYING)`, `position(ms, duration_ms)`, `track(title, artist, album)`, `error(message)`, `completed()` |
| `lookup(isrc)`, `lookup_by_title_artist(title, artist)` (with `load_uri`) | resolves music. Return a `bungalow.Track(uri, title, artist)`, just a URI, or `None` |
| `on_create()`, `on_start()`, `on_stop()`, `on_destroy()` | follows the service lifecycle |

Class attributes `id` (required), `name` and `icon` identify it.

Under the hood `PythonHost` wraps the module as a `com::IApplication` and each
service as a `com::IService`, so Python plugins go through the same
`PluginManager` and adapters as C++ ones. An exception raised by a plugin is
written to stderr and treated as a failed call; it never reaches the host.

The interpreter is isolated: it ignores environment variables and any Python
installed on the machine, and sees only the standard library shipped in
`<app>/python/Lib` (plus `Lib/site-packages` there, for extra packages). On
Windows Python itself is `python314.dll` next to the executable, and the
compiled standard modules (`socket`, `ssl`, `sqlite3`, …) are in
`<app>/python/DLLs`.

## Threading

- The host calls `IApplication`, lifecycle, sign-in and playback methods on
  its UI thread.
- `IMusicService` lookups run on the background worker and must be
  thread-safe.
- A plugin may call `ILoginCallback` and `IPlaybackListener` from any thread.
  The host moves them to the UI thread, and drops playback events that arrive
  after the service was removed.

## In the host

`PluginManager` loads each library, asks it for its `IApplication` and calls
`OnActivate` with a recording context. Every `IService` registered there is
wrapped by `wrapService()` (`ComServiceAdapter.h`) into a native `Service`,
`MediaService` or `MusicService` and added to the `ServiceManager`, so the
`PlaybackCoordinator` and the accounts view at `bungalow:accounts` treat it
like a built-in one. A plugin cannot take the id of a service that already
exists.

`setEnabled(id, false)` unregisters exactly the services the plugin added,
forgets their accounts and calls `OnDeactivate`. Libraries are unloaded when
the manager is destroyed, after every plugin object has been released.

Session data a plugin returns from `ISession::GetData` is encrypted by the
host as soon as it arrives (see `SPSession`).
