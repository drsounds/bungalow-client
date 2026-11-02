---
title: App bundles
sidebar_position: 1
---

# App bundles

An **app bundle** is a folder with a `manifest.json`. Bundles extend Bungalow
without recompiling it. A bundle can be:

* a **native app**: Lua controllers and XUL views rendered with Space elements
  ([Native apps](./native-apps.md)),
* an **HTML5 app**: an `index.html` run in an embedded browser
  ([HTML5 apps](./html5-apps.md)),
* a **framework** shared by other apps (the `import` bundle with the JavaScript
  API),
* a container for **extensions**, such as skins.

Bundles are managed by `space::SPAppManager` (`space/include/SPAppManager.h`).

## Search paths

At startup, `BungalowApp` adds the default search paths and scans them before
it creates the skin, because the skin may come from a bundle:

| Order | Path |
|-------|------|
| 1 | `<app>/Apps` (next to the executable) |
| 2 | `~/Bungalow` |
| 3 | `~/Bungalow/Apps` |
| 4 | `~/Documents/Bungalow` |
| 5 | `~/Documents/Bungalow/Apps` |

Paths are scanned in this order. **A bundle found later replaces an earlier one
with the same identifier**, so a bundle in your user folders overrides the
bundled one with the same id.

## The manifest

```json
{
    "BundleType": "Application",
    "BundleIdentifier": "myapp",
    "BundleVersion": "0.1.0",
    "AppName": { "en": "My App" },
    "AppDescription": { "en": "What it does." },
    "AppType": "Native",
    "AllowedHosts": ["api.example.com"],
    "ApiPermissions": ["io"],
    "Features": {
        "test": { "Routes": "feature/test/routes.json" }
    }
}
```

| Key | Meaning |
|-----|---------|
| `BundleType` | `Application`, `Framework`, `Skin`, … |
| `BundleIdentifier` | the bundle id. It is the host in `sp://<id>/…` and the id in `bungalow:app:<id>` |
| `BundleVersion` | optional version |
| `AppName`, `AppDescription` | display strings by language (`getName("en")`) |
| `AppType` | `"Native"` for a Lua app; otherwise a bundle with `index.html` is an HTML5 app |
| `AllowedHosts` | HTML5 apps: the https hosts the app may fetch from |
| `ApiPermissions` | native apps: extra Lua libraries (`"io"`, `"os"`) |
| `Features` | native apps: per-feature route files |

## Extensions

A bundle can contain extensions in subfolders, each with its own manifest. A
skin lives in `<bundle>/skins/<name>/manifest.json`:

```json
{
    "BundleType": "Skin",
    "BundleIdentifier": "modern.skin",
    "SkinName": { "en": "Modern Skin" }
}
```

`getExtensions("Skin")` lists every skin across all bundles, and
`findExtension(type, id)` looks one up. `createSkin("modern")` or
`createSkin("modern.skin")` loads it. See [Skins](../space/skins.md).

## `sp://` URIs

`sp://<bundle>/<path>` names a file inside a bundle.
`SPAppManager::resolve(uri)` turns it into an absolute path. It returns an
empty string if the bundle is unknown **or if the path would leave the bundle
folder**, so `..` can't be used to read other files. `resolveInBundle()` does
the same for a path relative to a bundle root and is thread-safe.

## Bundled apps

| Folder | Type | Purpose |
|--------|------|---------|
| `Apps/import` | Framework | the shared JavaScript Apps API, served at `sp://import/` |
| `Apps/native` | Application, native | a sample Lua app (`bungalow:app:native:<name>`) |
| `Apps/testapp` | Application, HTML5 | a sample HTML5 app |
| `Apps/modern` | Application | contains the `modern.skin` skin extension |

## Opening an app

`bungalow:app:<id>[:<segment>…]` opens an app. `AppView` (registered by
`WebConcept`) tries, in order:

1. a **native** bundle with that id, rendered by `SPNativeAppElement`,
2. a local **HTML5** bundle with an `index.html` (when CEF is enabled),
3. otherwise the hosted web app at `https://<id>.apps.bungalow.se`, shown like
   a site.

Installed apps can be bookmarked and then appear under **APPS** in the sidebar.
